#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <R.h>
#include <Rinternals.h>
#include <Rembedded.h>

#include "R_memory-object.h"
#include "R_magic.h"


int rdataToMemory(const char *filename, struct r_memoryobject_t *mem_obj)
{
  size_t iosz;
  FILE *fp;
  int64_t magic_size;

  // Open the file
  fp = fopen(filename, "rb");
  if (fp == 0) {
    fprintf(stderr, "fopen: (%d) %s\n", errno, strerror(errno));
    return errno;
  }

  // Read the magic from the file
  mem_obj->magic = R_ReadMagic(fp);
  if (mem_obj->magic == R_MAGIC_EMPTY || mem_obj->magic == R_MAGIC_CORRUPT) {
    fprintf(stderr, "R_ReadMagic: (%d) Error reading magic/magic corrupted\n", mem_obj->magic);
    fclose(fp);
    return EINVAL;
  }

  // Get the current position (ie length of the magic)
  magic_size = ftell(fp);
  if (magic_size <= 0) {
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
  mem_obj->buf_size = (size_t)(ftell(fp) - magic_size);
  if (mem_obj->buf_size <= 0) {
    fprintf(stderr, "ftell: (%d) %s\n", errno, strerror(errno));
    fclose(fp);
    return errno;
  }

  // Go back to the (nearly) the beginning of the file
  if (fseek(fp, magic_size, SEEK_SET) != 0) {
    fprintf(stderr, "fseek: (%d) %s\n", errno, strerror(errno));
    fclose(fp);
    return errno;
  }

  // Allocate a memory buffer for the file contents
  mem_obj->buf = (char *)malloc((mem_obj->buf_size) * sizeof(char));
  if (mem_obj->buf == 0) {
    fprintf(stderr, "malloc: (%d) %s\n", errno, strerror(errno));
    fclose(fp);
    return errno;
  }

  // Read the file contents to the memory buffer
  iosz = fread(mem_obj->buf, sizeof(char), mem_obj->buf_size, fp);
  if (iosz == 0) {
    fprintf(stderr, "fread: (%d) %s\n", errno, strerror(errno));
    fclose(fp);
    return errno;
  }
  if (iosz != mem_obj->buf_size) {
    fprintf(stderr, "fread: Not enough bytes (got %lu, expected %lu)\n", iosz, mem_obj->buf_size);
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
int memoryToRData(const char *filename, struct r_memoryobject_t mem_obj)
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
  if (R_WriteMagic(fp, mem_obj.magic) != 0) {
    fprintf(stderr, "R_WriteMagic: (%d) %s\n", errno, strerror(errno));
    fclose(fp);
    return errno;
  }

  // Write the file contents from the memory buffer
  iosz = fwrite(mem_obj.buf, sizeof(char), mem_obj.buf_size, fp);
  if (iosz == 0) {
    fprintf(stderr, "fwrite: (%d) %s\n", errno, strerror(errno));
    fclose(fp);
    return errno;
  }
  if (iosz != mem_obj.buf_size) {
    fprintf(stderr, "fwrite: Not enough bytes (wrote %lu, expected %lu)\n", iosz, mem_obj.buf_size);
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


SEXP memoryToRObject(struct r_memoryobject_t mem_obj)
{
  FILE *fp;
  struct R_inpstream_st rin;
  SEXP rObj;
  R_pstream_format_t format;

  switch (mem_obj.magic) {
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
      fprintf(stderr, "Bad R magic: %d\n", mem_obj.magic);
      return R_NilValue;
  }

  // Create a memory file device
  fp = fmemopen(mem_obj.buf, mem_obj.buf_size, "r");
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
struct r_memoryobject_t *robjectToMemory(SEXP r_obj, int magic)
{
  FILE *fp;
  int version;
  R_pstream_format_t format;
  struct R_outpstream_st rout;
  struct r_memoryobject_t mem_obj;
  struct r_memoryobject_t *mem_obj_ptr;

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
  mem_obj.magic = magic;

  // Create an in-memory file
  fp = open_memstream(&(mem_obj.buf), &(mem_obj.buf_size));
  if (fp == NULL) {
    fprintf(stderr, "open_memstream: (%d) %s\n", errno, strerror(errno));
    return NULL;
  }

  // Write to in-memory file
  R_InitFileOutPStream(&rout, fp, format, version, NULL, NULL);
  R_Serialize(r_obj, &rout);

  // Close the in-memory file
  if (fclose(fp) != 0) {
    fprintf(stderr, "fclose: (%d) %s\n", errno, strerror(errno));
    return NULL;
  }

  // All done
  mem_obj_ptr = (struct r_memoryobject_t *)malloc(sizeof(struct r_memoryobject_t));
  memcpy((void *)mem_obj_ptr, (void *)&mem_obj, sizeof(struct r_memoryobject_t));

  return mem_obj_ptr;
}


SEXP rdataToRObject(const char *filename)
{
  FILE *fp;
  int magic;
  struct R_inpstream_st rin;
  R_pstream_format_t format;
  SEXP r_obj;

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
  r_obj = Rf_protect(R_Unserialize(&rin));

  // Close the in-memory file device
  if (fclose(fp) != 0) {
    fprintf(stderr, "fclose: (%d) %s\n", errno, strerror(errno));
  }

  // If you got here, well done
  return r_obj;
}
int robjectToRData(const char *filename, SEXP r_obj, int magic)
{
  FILE *fp;
  int version;
  R_pstream_format_t format;
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
  R_Serialize(r_obj, &rout);

  // Close the file
  if (fclose(fp) != 0) {
    fprintf(stderr, "fclose: (%d) %s\n", errno, strerror(errno));
    return errno;
  }

  // If you got here, well done, have a zero
  return 0;
}
