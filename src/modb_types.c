#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "modb_types.h"
#include "modb_groups.h"
#include "modb_users.h"
#include "strext.h"


struct metadata_t *allocMetadata(void)
{
  struct metadata_t *metadata;

  metadata = malloc(sizeof(struct metadata_t));
  if (metadata == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
  }
  memset(metadata, 0, sizeof(struct metadata_t));

  return metadata;
}
struct metadata_t **allocMetadataList(size_t n_metadatas)
{
  struct metadata_t **metadatas;

  metadatas = (struct metadata_t **)malloc(sizeof(struct metadata_t *) * n_metadatas);
  if (metadatas == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }
  memset(metadatas, 0, sizeof(struct metadata_t *) * n_metadatas);

  return metadatas;
}
void freeMetadata(struct metadata_t **metadata_ptr)
{
  struct metadata_t *metadata = *metadata_ptr;

  if (metadata->ext != 0) {
    freeMetadataExt(&metadata->ext);
  }

  if (metadata->object != 0) {
    freeObject(&metadata->object);
  }

  if (metadata->n_groups > 0) {
    if (metadata->group_ids != 0) {
      free(metadata->group_ids);
      metadata->group_ids = 0;
    }
    if (metadata->groups != 0) {
      freeGroups(&metadata->groups, metadata->n_groups);
    }
    metadata->n_groups = 0;
  }

  if (metadata->owner != 0) {
    freeUser(&metadata->owner);
  }

  if (metadata->title != 0) {
    free(metadata->title);
    metadata->title = 0;
  }

  free(metadata);
  *metadata_ptr = 0;
}
void freeMetadataList(struct metadata_t ***metadata_list_ptr, size_t n_metadatas)
{
  size_t idx;
  struct metadata_t **metadatas_list= *metadata_list_ptr;

  for (idx = 0; idx < n_metadatas; idx++) {
    if (*(metadatas_list + idx) != 0) {
      freeMetadata((metadatas_list + idx));
    }
  }

  free(metadatas_list);
  *metadata_list_ptr = 0;
}

struct object_t *allocObject(void)
{
  struct object_t *object;

  object = malloc(sizeof(struct object_t));
  if (object == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
  }
  memset(object, 0, sizeof(struct object_t));

  return object;
}
struct object_t **allocObjects(size_t n_objects)
{
  struct object_t **objects;

  objects = (struct object_t **)malloc(sizeof(struct object_t *) * n_objects);
  if (objects == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }
  memset(objects, 0, sizeof(struct object_t *) * n_objects);

  return objects;
}
void freeObject(struct object_t **object_ptr)
{
  struct object_t *object = *object_ptr;

  if (object->data != 0) {
    free(object->data);
    object->data = 0;
  }

  free(object);
  *object_ptr = 0;
}
void freeObjects(struct object_t ***objects_ptr, size_t n_objects)
{
  size_t idx;
  struct object_t **objects = *objects_ptr;

  for (idx = 0; idx < n_objects; idx++) {
    if (*(objects + idx) != 0) {
      freeObject(objects + idx);
    }
  }

  free(objects);
  *objects_ptr = 0;
}

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
