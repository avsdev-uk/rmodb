#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "modb_types.h"
#include "strext.h"


struct modb_t *modbAlloc(const char *name, size_t name_len)
{
  struct modb_t *modb = (struct modb_t *)malloc(sizeof(struct modb_t));
  if (modb == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }

  if (strmemcpy(name, name_len, &modb->name, &modb->name_len) != 0) {
    modbRelease(&modb);
    return 0;
  }

  return modb;
}
void modbRelease(struct modb_t **modb)
{
  if ((*modb)->name != 0) {
    free((*modb)->name);
    (*modb)->name = 0;
  }

  free(*modb);
  *modb = 0;
}
