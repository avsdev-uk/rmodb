#ifndef H__MODB_TYPES__
#define H__MODB_TYPES__

#include <stddef.h>
#include <stdint.h>

struct modb_t {
  char *name;
  size_t name_len;
};

struct modb_t *modbAlloc(const char *name, size_t name_len);
void modbRelease(struct modb_t **modb);

#endif // H__MODB_TYPES__
