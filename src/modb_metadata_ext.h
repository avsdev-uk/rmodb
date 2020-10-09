#ifndef H__MODB_METADATA_EXT__
#define H__MODB_METADATA_EXT__

#include "modb_ref.h"


// MetadataExtended object
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

struct metadata_ext_t *allocMetadataExt(void);
struct metadata_ext_t **allocMetadataExts(size_t n_metadata_exts);
void freeMetadataExt(struct metadata_ext_t **metadata_ext);
void freeMetadataExts(struct metadata_ext_t ***metadata_exts_ptr, size_t n_metadata_exts);


#endif // H__MODB_METADATA_EXT__
