#ifndef __R_MAGIC_H__
#define __R_MAGIC_H__

#include <stdio.h>

#define R_STREAM_VERSION 3

#define R_MAGIC_ASCII_V3   3001
#define R_MAGIC_BINARY_V3  3002
#define R_MAGIC_XDR_V3     3003
#define R_MAGIC_ASCII_V2   2001
#define R_MAGIC_BINARY_V2  2002
#define R_MAGIC_XDR_V2     2003
#define R_MAGIC_ASCII_V1   1001
#define R_MAGIC_BINARY_V1  1002
#define R_MAGIC_XDR_V1     1003
#define R_MAGIC_EMPTY      999
#define R_MAGIC_CORRUPT    998
#define R_MAGIC_MAYBE_TOONEW 997

/* pre-1 formats (R < 0.99.0) */
#define R_MAGIC_BINARY 1975
#define R_MAGIC_ASCII  1976
#define R_MAGIC_XDR    1977
#define R_MAGIC_BINARY_VERSION16 1971
#define R_MAGIC_ASCII_VERSION16  1972

int R_WriteMagic(FILE *fp, int number);

int R_ReadMagic(FILE *fp);

#endif // __R_MAGIC_H__
