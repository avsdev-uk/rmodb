#ifndef H__MODB_REF__
#define H__MODB_REF__

#include <stddef.h>

struct modb_ref_t {
  const char *name;
  size_t name_len;
};
typedef struct modb_ref_t modb_ref;

#endif // H__MODB_REF__
