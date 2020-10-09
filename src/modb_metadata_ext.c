#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "modb_metadata_ext.h"


struct metadata_ext_t *allocMetadataExt(void)
{
  struct metadata_ext_t *metadata_ext;

  metadata_ext = malloc(sizeof(struct metadata_ext_t));
  if (metadata_ext == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
  }
  memset(metadata_ext, 0, sizeof(struct metadata_ext_t));

  return metadata_ext;
}
struct metadata_ext_t **allocMetadataExts(size_t n_metadata_exts)
{
  struct metadata_ext_t **metadata_exts;

  metadata_exts = (struct metadata_ext_t **)malloc(sizeof(struct metadata_ext_t *) * n_metadata_exts);
  if (metadata_exts == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }
  memset(metadata_exts, 0, sizeof(struct metadata_ext_t *) * n_metadata_exts);

  return metadata_exts;
}
void freeMetadataExt(struct metadata_ext_t **metadata_ext_ptr)
{
  struct metadata_ext_t *metadata_ext = *metadata_ext_ptr;

  free(metadata_ext);
  *metadata_ext_ptr = 0;
}
void freeMetadataExts(struct metadata_ext_t ***metadata_exts_ptr, size_t n_metadata_exts)
{
  size_t idx;
  struct metadata_ext_t **metadata_exts = *metadata_exts_ptr;

  for (idx = 0; idx < n_metadata_exts; idx++) {
    if (*(metadata_exts + idx) != 0) {
      freeMetadataExt(metadata_exts + idx);
    }
  }

  free(metadata_exts);
  *metadata_exts_ptr = 0;
}
