#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "modb_types.h"
#include "strext.h"


struct user_t *allocUser(void)
{
  struct user_t *user;

  user = malloc(sizeof(struct user_t));
  if (user == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
  }
  memset(user, 0, sizeof(struct user_t));

  return user;
}
struct user_t **allocUsers(size_t n_users)
{
  struct user_t **users;

  users = (struct user_t **)malloc(sizeof(struct user_t *) * n_users);
  if (users == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }
  memset(users, 0, sizeof(struct user_t *) * n_users);

  return users;
}
void freeUser(struct user_t **user_ptr)
{
  struct user_t *user = *user_ptr;

  if (user->n_groups > 0) {
    if (user->group_ids != 0) {
      free(user->group_ids);
      user->group_ids = 0;
    }
    if (user->groups != 0) {
      freeGroups(&user->groups, user->n_groups);
    }
    user->n_groups = 0;
  }

  if (user->email != 0) {
    free(user->email);
    user->email = 0;
  }

  if (user->username != 0) {
    free(user->username);
    user->username = 0;
  }

  free(user);
  *user_ptr = 0;
}
void freeUsers(struct user_t ***users_ptr, size_t n_users)
{
  size_t idx;
  struct user_t ** users = *users_ptr;

  for (idx = 0; idx < n_users; idx++) {
    if (*(users + idx) != 0) {
      freeUser((users + idx));
    }
  }

  free(users);
  *users_ptr = 0;
}


struct group_t *allocGroup(void)
{
  struct group_t *group;

  group = malloc(sizeof(struct group_t));
  if (group == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
  }
  memset(group, 0, sizeof(struct group_t));

  return group;
}
struct group_t **allocGroups(size_t n_groups)
{
  struct group_t **groups;

  groups = (struct group_t **)malloc(sizeof(struct group_t *) * n_groups);
  if (groups == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }
  memset(groups, 0, sizeof(struct group_t *) * n_groups);

  return groups;
}
void freeGroup(struct group_t **group_ptr)
{
  struct group_t *group = *group_ptr;

  if (group->n_members > 0) {
    if (group->member_ids != 0) {
      free(group->member_ids);
      group->member_ids = 0;
    }
    if (group->members != 0) {
      freeUsers(&group->members, group->n_members);
    }
    group->n_members = 0;
  }

  if (group->name != 0) {
    free(group->name);
    group->name = 0;
  }

  free(group);
  *group_ptr = 0;
}
void freeGroups(struct group_t ***groups_ptr, size_t n_groups)
{
  size_t idx;
  struct group_t **groups = *groups_ptr;

  for (idx = 0; idx < n_groups; idx++) {
    if (*(groups + idx) != 0) {
      freeGroup(groups + idx);
    }
  }

  free(groups);
  *groups_ptr = 0;
}


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
