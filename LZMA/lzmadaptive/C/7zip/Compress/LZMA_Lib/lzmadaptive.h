#ifndef __LZMA_LIB_H__
#define __LZMA_LIB_H__

#include <zlib.h>

// CJH: For Gentoo zlib compatibility
#ifdef _Z_OF
#undef OF
#define OF _Z_OF
#endif

/* CJH: Depreciated.
int lzmalib_uncompress OF((Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen));
int lzma7z_uncompress OF((Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen));
int lzmalinksys_uncompress OF((Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen));
*/

int lzmawrt_uncompress OF((Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen));
int lzmaspec_uncompress OF((Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int lc, int lp, int pb, int dictionary_size, int offset));

#endif
