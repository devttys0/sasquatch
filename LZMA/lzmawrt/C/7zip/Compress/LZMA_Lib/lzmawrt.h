#ifndef __LZMA_WRT_H__
#define __LZMA_WRT_H__

#include <zlib.h>

int lzmawrt_compress2 OF((Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level));
int lzmawrt_uncompress OF((Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen));

#endif
