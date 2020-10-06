#ifndef H__MODB_TYPES__
#define H__MODB_TYPES__

#include <stddef.h>
#include <stdint.h>

struct modb_ref_t {
  const char *name;
  size_t name_len;
};
typedef struct modb_ref_t modb_ref;


#endif // H__MODB_TYPES__
