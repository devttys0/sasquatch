/*
 * Copyright (c) 2009, 2010, 2013
 * Phillip Lougher <phillip@squashfs.org.uk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * lzma_wrapper.c
 *
 * Support for LZMA1 compression using LZMA SDK (4.65 used in
 * development, other versions may work) http://www.7-zip.org/sdk.html
 */

#include <LzmaLib.h>

#include "squashfs_fs.h"
#include "compressor.h"

// CJH: Added these includes
#include <stdio.h>
#include "error.h"
#include "7z.h"
#include "lzmalib.h"

#define LZMA_HEADER_SIZE	(LZMA_PROPS_SIZE + 8)

static int lzma_compress(void *strm, void *dest, void *src, int size, int block_size,
		int *error)
{
	unsigned char *d = dest;
	size_t props_size = LZMA_PROPS_SIZE,
		outlen = block_size - LZMA_HEADER_SIZE;
	int res;

	res = LzmaCompress(dest + LZMA_HEADER_SIZE, &outlen, src, size, dest,
		&props_size, 5, block_size, 3, 0, 2, 32, 1);
	
	if(res == SZ_ERROR_OUTPUT_EOF) {
		/*
		 * Output buffer overflow.  Return out of buffer space error
		 */
		return 0;
	}

	if(res != SZ_OK) {
		/*
		 * All other errors return failure, with the compressor
		 * specific error code in *error
		 */
		*error = res;
		return -1;
	}

	/*
	 * Fill in the 8 byte little endian uncompressed size field in the
	 * LZMA header.  8 bytes is excessively large for squashfs but
	 * this is the standard LZMA header and which is expected by the kernel
	 * code
	 */
	d[LZMA_PROPS_SIZE] = size & 255;
	d[LZMA_PROPS_SIZE + 1] = (size >> 8) & 255;
	d[LZMA_PROPS_SIZE + 2] = (size >> 16) & 255;
	d[LZMA_PROPS_SIZE + 3] = (size >> 24) & 255;
	d[LZMA_PROPS_SIZE + 4] = 0;
	d[LZMA_PROPS_SIZE + 5] = 0;
	d[LZMA_PROPS_SIZE + 6] = 0;
	d[LZMA_PROPS_SIZE + 7] = 0;

	/*
	 * Success, return the compressed size.  Outlen returned by the LZMA
	 * compressor does not include the LZMA header space
	 */
	return outlen + LZMA_HEADER_SIZE;
}

// CJH: s/lzma_uncompress/lzma_standard_uncompress/
static int lzma_standard_uncompress(void *dest, void *src, int size, int outsize,
	int *error)
{
	unsigned char *s = src;
	size_t outlen, inlen = size - LZMA_HEADER_SIZE;
	int res;

	outlen = s[LZMA_PROPS_SIZE] |
		(s[LZMA_PROPS_SIZE + 1] << 8) |
		(s[LZMA_PROPS_SIZE + 2] << 16) |
		(s[LZMA_PROPS_SIZE + 3] << 24);

	if(outlen > outsize)
    {
        /* CJH: Don't consider this an error, as many implementations omit the size field from the LZMA header
		*error = 0;
		return -1;
        */
        outlen = outsize;
        inlen = size - LZMA_PROPS_SIZE;
        TRACE("lzma_standard_uncompress: lzma data block does not appear to contain a valid size field\n");

	    res = LzmaUncompress(dest, &outlen, src + LZMA_PROPS_SIZE, &inlen, src, LZMA_PROPS_SIZE);
	}
    else
    {
	    res = LzmaUncompress(dest, &outlen, src + LZMA_HEADER_SIZE, &inlen, src, LZMA_PROPS_SIZE);
    }

	if(res == SZ_OK)
		return outlen;
	else {
		*error = res;
		return -1;
	}
}

// CJH: lzma_brcm variant decompressor
static int lzma_brcm_uncompress(void *dest, void *src, int size, int outsize, int *error)
{
    int retval = -1;

    if((retval = decompress_lzma_brcm((unsigned char *) src, (unsigned int) size, (unsigned char *) dest, (unsigned int) outsize)) != 0)
    {
        *error = retval;
        TRACE("decompress_lzma_brcm failed with error code %d\n", *error);
        return -1;
    }
    else
    {
        TRACE("decompress_lzma_brcm succeeded in decompressing %d bytes!\n", outsize);
        return outsize;
    }
}

// CJH: lzmawrt varient decompressor
static int lzma_wrt_uncompress(void *dest, void *src, int size, int outsize, int *error)
{
    int retval = -1;

    if((retval = lzmawrt_uncompress((Bytef *) dest, (uLongf *) &outsize, (const Bytef *) src, (uLong) size)) != 0)
    {
        *error = retval;
        retval = -1;
        TRACE("lzmawrt_uncompress failed with error code %d\n", *error);
    }
    else
    {
        TRACE("lzmawrt_uncompress succeeded: [%d] [%d]\n", retval, outsize);
        retval = outsize;
    }

    return retval;
}

struct lzma_props
{
    int lc;
    int lp;
    int pb;
    int offset;
    int detected;
};
struct lzma_props properties = { 0 };
extern struct override_table override;
static int lzma_adaptive_uncompress(void *dest, void *src, int size, int outsize, int *error)
{
    int lc, lp, pb, i, offset;
    int retval = -1, expected_outsize = 0;
    /*
     * Sometimes the LZMA data doesn't start at the beginning of src.
     * This can be due to a variety of reasons, usually because the
     * LZMA vendor implementation encoded the compression properties
     * into the first few bytes. These are the only offsets observed
     * in the wild and ordered by prevalence.
     */
    int offsets[4] = {0, 4, 9, 5};

    expected_outsize = outsize;

    // Properties already detected? Do it.
    if(properties.detected)
    {
        retval = lzmaspec_uncompress((Bytef *) dest, 
                                     (uLongf *) &outsize, 
                                     (const Bytef *) src, 
                                     (uLong) size,
                                     properties.lc,
                                     properties.lp,
                                     properties.pb,
                                     properties.offset);
    
        if(retval == 0)
        {
            return outsize;
        }
    }

    /*
     * Go through all possible combinations of lp, lc, pb and common LZMA data offsets.
     * Take the first valid decompression we can get.
     */
    for(i=0; i<4; i++)
    {
        /*
         * This is redundant if an override value is specified, but it
         * is a quick and easy way to let the user specify arbitrary offsets.
         */
        if(override.offset.set) offset = override.offset.value;
        else offset = offsets[i];

        for(lc=0; lc<=4; lc++)
        {
            if(override.lc.set && override.lc.value != lc) continue;

            for(lp=0; lp<=4; lp++)
            {
                if(override.lp.set && override.lp.value != lp) continue;

                for(pb=0; pb<=8; pb++)
                {
                    if(override.pb.set && override.pb.value != pb) continue;

                    outsize = expected_outsize;
                    retval = lzmaspec_uncompress((Bytef *) dest, 
                                                 (uLongf *) &outsize, 
                                                 (const Bytef *) src, 
                                                 (uLong) size,
                                                 lc,
                                                 lp,
                                                 pb,
                                                 offset);

                    /*
                     * If the decompression was successful, and if the expected decompressed
                     * size matches the expected decompressed size, then these decompression
                     * settings are likely valid.
                     *
                     * Note that for some data blocks, the code doesn't know the exact size
                     * beforehand, and simply sets the expected size to some known maximum
                     * value (SQUASHFS_FILE_SIZE, SQUASHFS_METADATA_SIZE, etc). The first
                     * things to be decompressed however are the fragment/inode/directory tables,
                     * and those have known sizes, making this a reasonably predictable way
                     * of recovering the LZMA compression options so long as the same options
                     * are used for all decompressed blocks (DD-WRT, for example, uses different
                     * options for each compressed block, but I've never seen it anywhere else).
                     */
                    if(retval == 0 &&
                       (expected_outsize == SQUASHFS_METADATA_SIZE ||
                        expected_outsize == SQUASHFS_FILE_SIZE ||
                        expected_outsize == outsize))
                    {
                        properties.lc = lc;
                        properties.lp = lp;
                        properties.pb = pb;
                        properties.offset = offsets[i];
                        properties.detected = 1;
                        
                        ERROR("Detected LZMA settings [lc: %d, lp: %d, pb: %d, offset: %d], ", properties.lc,
                                                                                               properties.lp,
                                                                                               properties.pb,
                                                                                               properties.offset);
                        ERROR("decompressed %d/%d bytes\n", outsize, expected_outsize);
                        return outsize;
                    }
                }
            }
        }
    }

    *error = retval;
    return -1;
}

struct compressor lzma_comp_ops = {
	.init = NULL,
	.compress = lzma_compress,
	.uncompress = lzma_standard_uncompress,
	.options = NULL,
	.usage = NULL,
	.id = LZMA_COMPRESSION,
	.name = "lzma",
	.supported = 1
};

struct compressor lzma_brcm_comp_ops = {
	.init = NULL,
	.compress = lzma_compress,
	.uncompress = lzma_brcm_uncompress,
	.options = NULL,
	.usage = NULL,
	.id = LZMA_BRCM_COMPRESSION,
	.name = "lzma-brcm",
	.supported = 1
};

struct compressor lzma_adaptive_comp_ops = {
	.init = NULL,
	.compress = lzma_compress,
	.uncompress = lzma_adaptive_uncompress,
	.options = NULL,
	.usage = NULL,
	.id = LZMA_ADAPTIVE_COMPRESSION,
	.name = "lzma-adaptive",
	.supported = 1
};

struct compressor lzma_wrt_comp_ops = {
	.init = NULL,
	.compress = lzma_compress,
	.uncompress = lzma_wrt_uncompress,
	.options = NULL,
	.usage = NULL,
	.id = LZMA_WRT_COMPRESSION,
	.name = "lzma-ddwrt",
	.supported = 1
};

