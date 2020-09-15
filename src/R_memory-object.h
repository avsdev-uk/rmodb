#ifndef H__R_MEMORY_OBJECT__
#define H__R_MEMORY_OBJECT__

#include <Rinternals.h>

struct r_memoryobject_t {
  char *buf;
  size_t buf_size;
  int magic;
};

int rdataToMemory(const char *filename, struct r_memoryobject_t *obj_mem);
int memoryToRData(const char *filename, struct r_memoryobject_t obj_mem);

SEXP memoryToRObject(struct r_memoryobject_t obj_mem);
struct r_memoryobject_t *robjectToMemory(SEXP r_obj, int magic);

SEXP rdataToRObject(const char *filename);
int robjectToRData(const char *filename, SEXP r_obj, int magic);

#endif // H__R_MEMORY_OBJECT__
