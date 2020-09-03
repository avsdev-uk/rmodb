#include "Rmagic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int R_WriteMagic(FILE *fp, int number)
{
  unsigned char buf[5];
  size_t res;

  number = abs(number);
  switch (number) {
    case R_MAGIC_ASCII_V1:   /* Version 1 - R Data, ASCII Format */
      strcpy((char*)buf, "RDA1");
    break;
    case R_MAGIC_BINARY_V1:  /* Version 1 - R Data, Binary Format */
      strcpy((char*)buf, "RDB1");
    break;
    case R_MAGIC_XDR_V1:     /* Version 1 - R Data, XDR Binary Format */
      strcpy((char*)buf, "RDX1");
    break;
    case R_MAGIC_ASCII_V2:   /* Version 2 - R Data, ASCII Format */
      strcpy((char*)buf, "RDA2");
    break;
    case R_MAGIC_BINARY_V2:  /* Version 2 - R Data, Binary Format */
      strcpy((char*)buf, "RDB2");
    break;
    case R_MAGIC_XDR_V2:     /* Version 2 - R Data, XDR Binary Format */
      strcpy((char*)buf, "RDX2");
    break;
    case R_MAGIC_ASCII_V3:   /* Version >=3 - R Data, ASCII Format */
      strcpy((char*)buf, "RDA3");
    break;
    case R_MAGIC_BINARY_V3:  /* Version >=3 - R Data, Binary Format */
      strcpy((char*)buf, "RDB3");
    break;
    case R_MAGIC_XDR_V3:     /* Version >=3 - R Data, XDR Binary Format */
      strcpy((char*)buf, "RDX3");
    break;
    default:
      buf[0] = (unsigned char)((number / 1000) % 10 + '0');
      buf[1] = (unsigned char)((number / 100) % 10 + '0');
      buf[2] = (unsigned char)((number / 10) % 10 + '0');
      buf[3] = (unsigned char)(number % 10 + '0');
  }
  buf[4] = '\n';

  res = fwrite((char*)buf, sizeof(char), 5, fp);
  if(res != 5) {
    return -1;
  }
  return 0;
}

int R_ReadMagic(FILE *fp)
{
  unsigned char buf[6];
  int d1, d2, d3, d4;
  size_t count;

  count = fread((char*)buf, sizeof(char), 5, fp);
  if (count != 5) {
    if (count == 0) {
      return R_MAGIC_EMPTY;
    } else {
      return R_MAGIC_CORRUPT;
    }
  }
 
  /* Version 1 */
  if (strncmp((char*)buf, "RDA1\n", 5) == 0) {
    return R_MAGIC_ASCII_V1;
  } else if (strncmp((char*)buf, "RDB1\n", 5) == 0) {
    return R_MAGIC_BINARY_V1;
  } else if (strncmp((char*)buf, "RDX1\n", 5) == 0) {
    return R_MAGIC_XDR_V1;
  }

  /* Version 2 */
  if (strncmp((char*)buf, "RDA2\n", 5) == 0) {
    return R_MAGIC_ASCII_V2;
  } else if (strncmp((char*)buf, "RDB2\n", 5) == 0) {
    return R_MAGIC_BINARY_V2;
  } else if (strncmp((char*)buf, "RDX2\n", 5) == 0) {
    return R_MAGIC_XDR_V2;
  }

  /* Version 3 */
  if (strncmp((char*)buf, "RDA3\n", 5) == 0) {
    return R_MAGIC_ASCII_V3;
  } else if (strncmp((char*)buf, "RDB3\n", 5) == 0) {
    return R_MAGIC_BINARY_V3;
  } else if (strncmp((char*)buf, "RDX3\n", 5) == 0) {
    return R_MAGIC_XDR_V3;
  } else if (strncmp((char *)buf, "RD", 2) == 0) {
    return R_MAGIC_MAYBE_TOONEW;
  }

  /* Intel gcc seems to screw up a single expression here */
  d1 = (buf[3] - '0') % 10;
  d2 = (buf[2] - '0') % 10;
  d3 = (buf[1] - '0') % 10;
  d4 = (buf[0] - '0') % 10;
  return d1 + 10 * d2 + 100 * d3 + 1000 * d4;
}