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

// CJH: LZMA variant list
#define LZMA_STANDARD       1
#define LZMA_7Z             2
#define LZMA_WRT            3   
#define LZMA_ADAPTIVE       4
#define LZMA_VARIANTS_COUNT LZMA_ADAPTIVE

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

// CJH: s/lzma_uncompress/standard_lzma_uncompress/
static int standard_lzma_uncompress(void *dest, void *src, int size, int outsize,
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
        TRACE("standard_lzma_uncompress: lzma data block does not appear to contain a valid size field\n");

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

// CJH: lzma_7z variant decompressor
int lzma_7z_uncompress(void *dest, void *src, int size, int outsize, int *error)
{
    int retval = -1;

    if((retval = decompress_lzma_7z((unsigned char *) src, (unsigned int) size, (unsigned char *) dest, (unsigned int) outsize)) != 0)
    {
        *error = retval;
        TRACE("decompress_lzma_7z failed with error code %d\n", *error);
        return -1;
    }
    else
    {
        TRACE("decompress_lzma_7z succeeded in decompressing %d bytes!\n", outsize);
        return outsize;
    }
}

// CJH: lzmawrt varient decompressor
extern int ddwrt_squash_image;
int lzma_wrt_uncompress(void *dest, void *src, int size, int outsize, int *error)
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
static int lzma_adaptive_uncompress(void *dest, void *src, int size, int outsize, int *error)
{
    int lc, lp, pb, i;
    int offsets[4] = {0, 4, 5, 9};
    int retval = -1, expected_outsize = 0;

    expected_outsize = outsize;

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

    for(lc=0; lc<=4; lc++)
    {
        for(lp=0; lp<=4; lp++)
        {
            for(pb=0; pb<=8; pb++)
            {
                for(i=0; i<4; i++)
                {
                    outsize = expected_outsize;
                    retval = lzmaspec_uncompress((Bytef *) dest, 
                                                 (uLongf *) &outsize, 
                                                 (const Bytef *) src, 
                                                 (uLong) size,
                                                 lc,
                                                 lp,
                                                 pb,
                                                 offsets[i]);

                    if(retval == 0)
                    {
                        properties.lc = lc;
                        properties.lp = lp;
                        properties.pb = pb;
                        properties.offset = offsets[i];
                        properties.detected = 1;
                        
                        ERROR("Detected LZMA settings [lc: %d, lp: %d, pb: %d, offset: %d]\n", properties.lc,
                                                                                               properties.lp,
                                                                                               properties.pb,
                                                                                               properties.offset);
                        return outsize;
                    }
                }
            }
        }
    }

    *error = retval;
    return -1;
}

// CJH: A decompression wrapper for the various LZMA versions
int detected_lzma_variant = -1;
static int lzma_uncompress(void *dest, void *src, int size, int outsize, int *error)
{
    int i = 0, k = 0, retval = -1;
    int lzma_variants[LZMA_VARIANTS_COUNT] = { 0 };

    if(detected_lzma_variant != -1)
    {
        lzma_variants[i] = detected_lzma_variant;
        i++;
    }

    for(k=1; i<LZMA_VARIANTS_COUNT; i++,k++)
    {
        if(k == detected_lzma_variant)
        {
            k++;
        }
        lzma_variants[i] = k;
    }

    for(i=0; (i<LZMA_VARIANTS_COUNT && retval < 1); i++)
    {
        if(detected_lzma_variant == -1) ERROR("Trying LZMA variant #%d\n", lzma_variants[i]);

        switch(lzma_variants[i])
        {
            case LZMA_STANDARD:
                retval = standard_lzma_uncompress(dest, src, size, outsize, error);
                break;
            case LZMA_7Z:
                retval = lzma_7z_uncompress(dest, src, size, outsize, error);
                break;
            case LZMA_WRT:
                retval = lzma_wrt_uncompress(dest, src, size, outsize, error);
                break;
            case LZMA_ADAPTIVE:
                retval = lzma_adaptive_uncompress(dest, src, size, outsize, error);
                break;
        }

        if(retval > 0 && detected_lzma_variant != lzma_variants[i])
        {
            detected_lzma_variant = lzma_variants[i];
            ERROR("Detected LZMA variant #%d\n", detected_lzma_variant);
        }
    }
    
    return retval;
}

struct compressor lzma_comp_ops = {
	.init = NULL,
	.compress = lzma_compress,
	.uncompress = lzma_uncompress,
	.options = NULL,
	.usage = NULL,
	.id = LZMA_COMPRESSION,
	.name = "lzma",
	.supported = 1
};

