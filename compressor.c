/*
 *
 * Copyright (c) 2009, 2010, 2011
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
 * compressor.c
 */

#include <stdio.h>
#include <string.h>
#include "compressor.h"
#include "squashfs_fs.h"

// CJH: Added these includes
#include "error.h"

#ifndef GZIP_SUPPORT
static struct compressor gzip_comp_ops =  {
	ZLIB_COMPRESSION, "gzip"
};
#else
extern struct compressor gzip_comp_ops;
#endif

#ifndef LZMA_SUPPORT
static struct compressor lzma_comp_ops = {
	LZMA_COMPRESSION, "lzma"
};
#else
extern struct compressor lzma_comp_ops;
#endif

#ifndef LZO_SUPPORT
static struct compressor lzo_comp_ops = {
	LZO_COMPRESSION, "lzo"
};
#else
extern struct compressor lzo_comp_ops;
#endif

#ifndef LZ4_SUPPORT
static struct compressor lz4_comp_ops = {
	LZ4_COMPRESSION, "lz4"
};
#else
extern struct compressor lz4_comp_ops;
#endif

#ifndef XZ_SUPPORT
static struct compressor xz_comp_ops = {
	XZ_COMPRESSION, "xz"
};
#else
extern struct compressor xz_comp_ops;
#endif

extern struct compressor lzma_alt_comp_ops;
extern struct compressor lzma_wrt_comp_ops;
extern struct compressor lzma_adaptive_comp_ops;

static struct compressor unknown_comp_ops = {
	0, "unknown"
};


struct compressor *compressor[] = {
	&gzip_comp_ops,
	&lzma_comp_ops,
    // CJH: Added additional LZMA decompressors. Order is intentional.
    &lzma_adaptive_comp_ops,
    &lzma_alt_comp_ops,
    &lzma_wrt_comp_ops,
	&lzo_comp_ops,
	&lz4_comp_ops,
	&xz_comp_ops,
	&unknown_comp_ops
};


int lookup_compressor_index(char *name)
{
    int i;

    for(i = 0; compressor[i]->id; i++)
    {
        if(strcmp(name, compressor[i]->name) == 0)
            return i;
    }

    return -1;
}

struct compressor *lookup_compressor(char *name)
{
	int i;

	for(i = 0; compressor[i]->id; i++)
		if(strcmp(compressor[i]->name, name) == 0)
			break;

	return compressor[i];
}


struct compressor *lookup_compressor_id(int id)
{
	int i;

	for(i = 0; compressor[i]->id; i++)
		if(id == compressor[i]->id)
			break;

	return compressor[i];
}


void display_compressors(char *indent, char *def_comp)
{
	int i;

	for(i = 0; compressor[i]->id; i++)
		if(compressor[i]->supported)
			fprintf(stderr, "%s\t%s%s\n", indent,
				compressor[i]->name,
				strcmp(compressor[i]->name, def_comp) == 0 ?
				" (default)" : "");
}


void display_compressor_usage(char *def_comp)
{
	int i;

	for(i = 0; compressor[i]->id; i++)
		if(compressor[i]->supported) {
			char *str = strcmp(compressor[i]->name, def_comp) == 0 ?
				" (default)" : "";
			if(compressor[i]->usage) {
				fprintf(stderr, "\t%s%s\n",
					compressor[i]->name, str);
				compressor[i]->usage();
			} else
				fprintf(stderr, "\t%s (no options)%s\n",
					compressor[i]->name, str);
		}
}

// CJH: calls the currently selected decompressor, unless that fails, then tries the other decompressors
int detected_compressor_index = 0;
int compressor_uncompress(struct compressor *comp, void *dest, void *src, int size, int block_size, int *error)
{
    int i = 0, retval = -1, default_compressor_id = -1;

    if(detected_compressor_index)
    {
        retval = compressor[detected_compressor_index]->uncompress(dest, src, size, block_size, error);
    }

    if(retval < 1 && comp->uncompress)
    {
        if(!detected_compressor_index) ERROR("Trying to decompress using default %s decompressor...\n", comp->name);
        
        retval = comp->uncompress(dest, src, size, block_size, error);
        
        if(!detected_compressor_index)
        {
            if(retval > 0)
            {
                ERROR("Successfully decompressed with default %s decompressor\n", comp->name);
                detected_compressor_index = lookup_compressor_index(comp->name);
            }
            else
            {
                TRACE("Default %s decompressor failed! [%d %d]\n", comp->name, retval, *error);
            }
        }
    }

    if(retval < 1)
    {
        default_compressor_id = comp->id;

        for(i=0; compressor[i]->id; i++)
        {
            comp = compressor[i];
            
            if(comp->id != default_compressor_id && 
               comp->id != compressor[detected_compressor_index]->id && 
               comp->uncompress)
            {
                ERROR("Trying to decompress with %s...\n", comp->name);
                retval = comp->uncompress(dest, src, size, block_size, error);
                if(retval > 0)
                {
                    //TRACE("%s decompressor succeeded!\n", comp->name);
                    ERROR("Detected %s compression\n", comp->name);
                    detected_compressor_index = i;
                    break;
                }
                else
                {
                    TRACE("%s decompressor failed! [%d %d]\n", comp->name, retval, *error);
                }
            }
        }
    }

    return retval;
}

