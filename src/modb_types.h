#ifndef H__MODB_TYPES__
#define H__MODB_TYPES__

#include <stddef.h>
#include <stdint.h>

struct modb_ref_t {
  const char *name;
  size_t name_len;
};
typedef struct modb_ref_t modb_ref;

struct metadata_t {
  unsigned int id;

  char *type;
  size_t type_len;

  char *title;
  size_t title_len;

  unsigned int owner_id;
  struct user_t *owner;

  int64_t created_on;
  int64_t updated_on;
  int64_t deleted_on;

  unsigned int *group_ids;
  struct group_t **groups;
  size_t n_groups;

  struct object_t *object;
  struct metadata_ext_t *ext;
};

struct object_t {
  unsigned int id;

  char *data;
  size_t data_len;
};

struct metadata_ext_t {
  unsigned int id;

// TODO: dynamic extended metadata definition
//  struct metadata_ext_value_t **meta_ext;
//  size_t n_meta_ext;
};

//struct metadata_ext_value_t {
//  char *key;
//  enum e_column_type_t type;
//  union {
//    void *vptr;_ptr

//    int8_t int8;
//    uint8_t uint8;

//    int16_t int16;
//    uint16_t uint16;

//    int32_t int32;
//    uint32_t uint32;

//    int64_t int64;
//    uint64_t uint64;

//    float flt;
//    double dbl;

//    char *ptr_str;
//  } value;
//  size_t value_len;
//};

struct metadata_t *allocMetadata(void);
struct metadata_t **allocMetadataList(size_t n_metadatas);
void freeMetadata(struct metadata_t **metadata);
void freeMetadataList(struct metadata_t ***metadata_list_ptr, size_t n_metadatas);

struct object_t *allocObject(void);
struct object_t **allocObjects(size_t n_objects);
void freeObject(struct object_t **object);
void freeObjects(struct object_t ***objects_ptr, size_t n_objects);

struct metadata_ext_t *allocMetadataExt(void);
struct metadata_ext_t **allocMetadataExts(size_t n_metadata_exts);
void freeMetadataExt(struct metadata_ext_t **metadata_ext);
void freeMetadataExts(struct metadata_ext_t ***metadata_exts_ptr, size_t n_metadata_exts);


#endif // H__MODB_TYPES__
