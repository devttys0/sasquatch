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
#include "7z.h"
#include "sqlzma.h"

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
        fprintf(stderr, "standard_lzma_uncompress: lzma data block does not appear to contain a valid size field\n");

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

// CJH: A decompression wrapper for the various LZMA versions
struct sqlzma_un un = { 0 };
static int lzma_uncompress(void *dest, void *src, int size, int outsize, int *error)
{
    int retval = -1;

    if((retval = standard_lzma_uncompress(dest, src, size, outsize, error)) != 0)
    {
        fprintf(stderr, "standard_lzma_uncompress failed with error code %d\n", *error);
    }
    else
    {
        return retval;
    }
    
    if((retval = decompress_lzma_7z((unsigned char *) src, (unsigned int) size, (unsigned char *) dest, (unsigned int) outsize)) != 0)
    {
        *error = retval;
        fprintf(stderr, "decompress_lzma_7z failed with error code %d\n", *error);
    }
    else
    {
        fprintf(stderr, "decompress_lzma_7z succeeded in decompressing %d bytes!\n", outsize);
        return outsize;
    }

    if(!un.un_lzma)
    {
        un.un_lzma = 1;
        if(sqlzma_init(&un, un.un_lzma, 0) != 0)
        {
            fprintf(stderr, "sqlzma_init failed!\n");
            un.un_lzma = 0;
        }
    }
    if(un.un_lzma)
    {
        enum {Src, Dst};
        struct sized_buf sbuf[] = {
                {.buf = (void *)src, .sz = size},
                {.buf = (void *)dest, .sz = outsize}
        };
        if((retval = sqlzma_un(&un, sbuf+Src, sbuf+Dst)) != 0)
        {
            *error = retval;
            fprintf(stderr, "sqlzma_un failed with error code %d\n", *error);
            retval = -1;
        }
        else
        {
            fprintf(stderr, "sqlzma_un succeeded in decompressing %d bytes!\n", (int) un.un_reslen);
            return un.un_reslen;
        }
    }
    
    return -1;
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

