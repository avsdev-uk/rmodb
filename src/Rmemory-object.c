#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <R.h>
#include <Rinternals.h>
#include <Rembedded.h>

#include "Rmagic.h"
#include "Rmemory-object.h"


int rdataToMemory(const char *filename, struct rmemoryobject_t *memObj)
{
  size_t iosz;
  FILE *fp;
  int magicSize;

  // Open the file
  fp = fopen(filename, "rb");
  if (fp == 0) {
    fprintf(stderr, "fopen: (%d) %s\n", errno, strerror(errno));
    return errno;
  }

  // Read the magic from the file
  memObj->magic = R_ReadMagic(fp);
  if (memObj->magic == R_MAGIC_EMPTY || memObj->magic == R_MAGIC_CORRUPT) {
    fprintf(stderr, "R_ReadMagic: (%d) Error reading magic/magic corrupted\n", memObj->magic);
    fclose(fp);
    return EINVAL;
  }

  // Get the current position (ie length of the magic)
  magicSize = ftell(fp);
  if (magicSize <= 0) {
    fprintf(stderr, "ftell: (%d) %s\n", errno, strerror(errno));
    fclose(fp);
    return errno;
  }

  // Seek to the end of the file
  if (fseek(fp, 0, SEEK_END) != 0) {
    fprintf(stderr, "fseek: (%d) %s\n", errno, strerror(errno));
    fclose(fp);
    return errno;
  }

  // Get the current position (ie length of the file + magic)
  memObj->bufsz = ftell(fp) - magicSize;
  if (memObj->bufsz <= 0) {
    fprintf(stderr, "ftell: (%d) %s\n", errno, strerror(errno));
    fclose(fp);
    return errno;
  }

  // Go back to the (nearly) the beginning of the file
  if (fseek(fp, magicSize, SEEK_SET) != 0) {
    fprintf(stderr, "fseek: (%d) %s\n", errno, strerror(errno));
    fclose(fp);
    return errno;
  }

  // Allocate a memory buffer for the file contents
  memObj->buf = (char *)malloc((memObj->bufsz) * sizeof(char));
  if (memObj->buf == 0) {
    fprintf(stderr, "malloc: (%d) %s\n", errno, strerror(errno));
    fclose(fp);
    return errno;
  }

  // Read the file contents to the memory buffer
  iosz = fread(memObj->buf, sizeof(char), memObj->bufsz, fp);
  if (iosz == 0) {
    fprintf(stderr, "fread: (%d) %s\n", errno, strerror(errno));
    fclose(fp);
    return errno;
  }
  if (iosz != memObj->bufsz) {
    fprintf(stderr, "fread: Not enough bytes (got %lu, expected %lu)\n", iosz, memObj->bufsz);
    fclose(fp);
    return ENODATA;
  }

  // Close the file
  if (fclose(fp) != 0) {
    fprintf(stderr, "fclose: (%d) %s\n", errno, strerror(errno));
    return errno;
  }

  // If you got here, well done, have a zero
  return 0;
}
int memoryToRData(const char *filename, struct rmemoryobject_t memObj)
{
  size_t iosz;
  FILE *fp;

  // Open the file
  fp = fopen(filename, "wb");
  if (fp == 0) {
    fprintf(stderr, "fopen: (%d) %s\n", errno, strerror(errno));
    return errno;
  }

  // Write the R magic
  if (R_WriteMagic(fp, memObj.magic) != 0) {
    fprintf(stderr, "R_WriteMagic: (%d) %s\n", errno, strerror(errno));
    fclose(fp);
    return errno;
  }

  // Write the file contents from the memory buffer
  iosz = fwrite(memObj.buf, sizeof(char), memObj.bufsz, fp);
  if (iosz == 0) {
    fprintf(stderr, "fwrite: (%d) %s\n", errno, strerror(errno));
    fclose(fp);
    return errno;
  }
  if (iosz != memObj.bufsz) {
    fprintf(stderr, "fwrite: Not enough bytes (wrote %lu, expected %lu)\n", iosz, memObj.bufsz);
    fclose(fp);
    return ENODATA;
  }

  // Close the file
  if (fclose(fp) != 0) {
    fprintf(stderr, "fclose: (%d) %s\n", errno, strerror(errno));
    return errno;
  }

  // If you got here, well done, have a zero
  return 0;
}


SEXP memoryToRObject(struct rmemoryobject_t memObj)
{
  FILE *fp;
  struct R_inpstream_st rin;
  SEXP rObj;
  int format;

  switch (memObj.magic) {
    case R_MAGIC_ASCII_V2:
    case R_MAGIC_ASCII_V3:
      format = R_pstream_ascii_format;
      break;

    case R_MAGIC_BINARY_V2:
    case R_MAGIC_BINARY_V3:
      format = R_pstream_binary_format;
      break;

    case R_MAGIC_XDR_V2:
    case R_MAGIC_XDR_V3:
      format = R_pstream_xdr_format;
      break;

    default:
      fprintf(stderr, "Bad R magic: %d\n", memObj.magic);
      return R_NilValue;
  }

  // Create a memory file device
  fp = fmemopen(memObj.buf, memObj.bufsz, "r");
  if (fp == NULL) {
    fprintf(stderr, "fmemopen: (%d) %s\n", errno, strerror(errno));
    return R_NilValue;
  }

  // Read the stream
  R_InitFileInPStream(&rin, fp, format, NULL, NULL);
  rObj = Rf_protect(R_Unserialize(&rin));

  // Close the in-memory file device
  if (fclose(fp) != 0) {
    fprintf(stderr, "fclose: (%d) %s\n", errno, strerror(errno));
  }

  // All done
  Rf_unprotect(1);
  return rObj;
}
struct rmemoryobject_t *robjectToMemory(SEXP rObj, int magic)
{
  FILE *fp;
  int version;
  int format;
  struct R_outpstream_st rout;
  struct rmemoryobject_t memObj;
  struct rmemoryobject_t *memObjPtr;

  // Get version and format from the R magic header
  switch (magic) {
    case R_MAGIC_ASCII_V2:
    case R_MAGIC_BINARY_V2:
    case R_MAGIC_XDR_V2:
      version = 2;
      break;

    case R_MAGIC_ASCII_V3:
    case R_MAGIC_BINARY_V3:
    case R_MAGIC_XDR_V3:
      version = 3;
      break;

    default:
      fprintf(stderr, "Bad R magic: %d\n", magic);
      return NULL;
  }
  switch (magic) {
    case R_MAGIC_ASCII_V2:
    case R_MAGIC_ASCII_V3:
      format = R_pstream_ascii_format;
      break;

    case R_MAGIC_BINARY_V2:
    case R_MAGIC_BINARY_V3:
      format = R_pstream_binary_format;
      break;

    case R_MAGIC_XDR_V2:
    case R_MAGIC_XDR_V3:
      format = R_pstream_xdr_format;
      break;

    default:
      fprintf(stderr, "Bad R magic: %d\n", magic);
      return NULL;
  }
  memObj.magic = magic;

  // Create an in-memory file
  fp = open_memstream(&(memObj.buf), &(memObj.bufsz));
  if (fp == NULL) {
    fprintf(stderr, "open_memstream: (%d) %s\n", errno, strerror(errno));
    return NULL;
  }

  // Write to in-memory file
  R_InitFileOutPStream(&rout, fp, format, version, NULL, NULL);
  R_Serialize(rObj, &rout);

  // Close the in-memory file
  if (fclose(fp) != 0) {
    fprintf(stderr, "fclose: (%d) %s\n", errno, strerror(errno));
    return NULL;
  }

  // All done
  memObjPtr = (struct rmemoryobject_t *)malloc(sizeof(struct rmemoryobject_t));
  memcpy((void *)memObjPtr, (void *)&memObj, sizeof(struct rmemoryobject_t));

  return memObjPtr;
}


SEXP rdataToRObject(const char *filename)
{
  size_t iosz;
  FILE *fp;
  int magic;
  struct R_inpstream_st rin;
  int format;
  SEXP rObj;

  // Open the file
  fp = fopen(filename, "rb");
  if (fp == 0) {
    fprintf(stderr, "fopen: (%d) %s\n", errno, strerror(errno));
    return R_NilValue;
  }

  // Read the magic from the file
  magic = R_ReadMagic(fp);
  if (magic == R_MAGIC_EMPTY || magic == R_MAGIC_CORRUPT) {
    fprintf(stderr, "R_ReadMagic: (%d) Error reading magic/magic corrupted\n", magic);
    fclose(fp);
    return R_NilValue;
  }
  switch (magic) {
    case R_MAGIC_ASCII_V2:
    case R_MAGIC_ASCII_V3:
      format = R_pstream_ascii_format;
      break;

    case R_MAGIC_BINARY_V2:
    case R_MAGIC_BINARY_V3:
      format = R_pstream_binary_format;
      break;

    case R_MAGIC_XDR_V2:
    case R_MAGIC_XDR_V3:
      format = R_pstream_xdr_format;
      break;

    default:
      fprintf(stderr, "Bad R magic: %d\n", magic);
      return R_NilValue;
  }

  // Read the stream
  R_InitFileInPStream(&rin, fp, format, NULL, NULL);
  rObj = Rf_protect(R_Unserialize(&rin));

  // Close the in-memory file device
  if (fclose(fp) != 0) {
    fprintf(stderr, "fclose: (%d) %s\n", errno, strerror(errno));
  }

  // If you got here, well done
  return rObj;
}
int robjectToRData(const char *filename, SEXP rObj, int magic)
{
  size_t iosz;
  FILE *fp;
  int version;
  int format;
  struct R_outpstream_st rout;

  // Get version and format from the R magic
  switch (magic) {
    case R_MAGIC_ASCII_V2:
    case R_MAGIC_BINARY_V2:
    case R_MAGIC_XDR_V2:
      version = 2;
      break;

    case R_MAGIC_ASCII_V3:
    case R_MAGIC_BINARY_V3:
    case R_MAGIC_XDR_V3:
      version = 3;
      break;

    default:
      fprintf(stderr, "Bad R magic: %d\n", magic);
      return EINVAL;
  }
  switch (magic) {
    case R_MAGIC_ASCII_V2:
    case R_MAGIC_ASCII_V3:
      format = R_pstream_ascii_format;
      break;

    case R_MAGIC_BINARY_V2:
    case R_MAGIC_BINARY_V3:
      format = R_pstream_binary_format;
      break;

    case R_MAGIC_XDR_V2:
    case R_MAGIC_XDR_V3:
      format = R_pstream_xdr_format;
      break;

    default:
      fprintf(stderr, "Bad R magic: %d\n", magic);
      return EINVAL;
  }

  // Open the file
  fp = fopen(filename, "wb");
  if (fp == 0) {
    fprintf(stderr, "fopen: (%d) %s\n", errno, strerror(errno));
    return errno;
  }

  // Write the R magic
  if (R_WriteMagic(fp, magic) != 0) {
    fprintf(stderr, "R_WriteMagic: (%d) %s\n", errno, strerror(errno));
    fclose(fp);
    return errno;
  }

  // Write to file
  R_InitFileOutPStream(&rout, fp, format, version, NULL, NULL);
  R_Serialize(rObj, &rout);

  // Close the file
  if (fclose(fp) != 0) {
    fprintf(stderr, "fclose: (%d) %s\n", errno, strerror(errno));
    return errno;
  }

  // If you got here, well done, have a zero
  return 0;
}