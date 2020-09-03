#ifndef __R_MEMORY_OBJECT_H__
#define __R_MEMORY_OBJECT_H__

#include <Rinternals.h>

struct rmemoryobject_t {
  char *buf;
  size_t bufsz;
  int magic;
};

int rdataToMemory(const char *filename, struct rmemoryobject_t *objMem);
int memoryToRData(const char *filename, struct rmemoryobject_t objMem);

SEXP memoryToRObject(struct rmemoryobject_t objMem);
struct rmemoryobject_t *robjectToMemory(SEXP rObj, int magic);

SEXP rdataToRObject(const char *filename);
int robjectToRData(const char *filename, SEXP rObj, int magic);

#endif // __R_MEMORY_OBJECT_H__