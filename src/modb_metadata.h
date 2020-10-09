#ifndef H__MODB_METADATA__
#define H__MODB_METADATA__

#include "database.h"
#include "modb_ref.h"


// Metadata object
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

struct metadata_t *allocMetadata(void);
struct metadata_t **allocMetadataList(size_t n_metadatas);
void freeMetadata(struct metadata_t **metadata);
void freeMetadataList(struct metadata_t ***metadata_list_ptr, size_t n_metadatas);


// MODB Metadata
int modbMetadataById(stored_conn *sconn, modb_ref *modb, unsigned int id,
                     struct metadata_t **metadata);

int modbMetadataListByOwnerId(stored_conn *sconn, modb_ref *modb, unsigned int owner_id,
                              int with_deleted,
                              struct metadata_t ***metadata_list, size_t *n_metadatas);
int modbMetadataListByGroupId(stored_conn *sconn, modb_ref *modb, unsigned int group_id,
                              int with_deleted,
                              struct metadata_t ***metadata_list, size_t *n_metadatas);

int modbMetadataList(stored_conn *sconn, modb_ref *modb, int with_deleted,
                     struct metadata_t ***metadata_list, size_t *n_metadatas);

int64_t modbMetadataCreate(stored_conn *sconn, modb_ref *modb,
                           const struct metadata_t *const metadata);

int64_t modbMetadataReplace(stored_conn *sconn, modb_ref *modb, unsigned int id,
                            const struct metadata_t *const metadata);
int64_t modbMetadataUpdateType(stored_conn *sconn, modb_ref *modb, unsigned int id,
                               const char *type, size_t type_len);
int64_t modbMetadataUpdateTitle(stored_conn *sconn, modb_ref *modb, unsigned int id,
                                const char *title, size_t title_len);
int64_t modbMetadataUpdateOwner(stored_conn *sconn, modb_ref *modb, unsigned int id,
                                struct user_t *owner);
int64_t modbMetadataUpdateOwnerId(stored_conn *sconn, modb_ref *modb, unsigned int id,
                                  unsigned int owner_id);

int modbMetadataDelete(stored_conn *sconn, modb_ref *modb, unsigned int id);
int modbMetadataDestroy(stored_conn *sconn, modb_ref *modb, unsigned int id);


// MODB Metadata -> Owner
int64_t modbFetchMetadataOwner(stored_conn *sconn, modb_ref *modb, struct metadata_t *metadata);

// MODB Metadata -> Object
int64_t modbFetchMetadataObject(stored_conn *sconn, modb_ref *modb, struct metadata_t *metadata);

// MODB Metadata -> MetadataExtended
int64_t modbFetchMetadataExtended(stored_conn *sconn, modb_ref *modb, struct metadata_t *metadata);

// MODB Metadata -> Groups
int modbFetchMetadataGroupIds(stored_conn *sconn, modb_ref *modb,
                              struct metadata_t *metadata, int with_deleted);
int modbFetchMetadataGroups(stored_conn *sconn, modb_ref *modb, struct metadata_t *metadata,
                            int with_deleted);

int modbSyncMetadataGroups(stored_conn *sconn, modb_ref *modb,
                           unsigned int metadata_id, size_t n_groups, unsigned int *group_id);
int modbSyncMetadataGroups_va(stored_conn *sconn, modb_ref *modb,
                              unsigned int metadata_id, size_t n_groups, ...);

int modbIsLinked_Metadata_Group(stored_conn *sconn, modb_ref *modb,
                                unsigned int metadata_id, unsigned int group_id);
int modbLink_Metadata_Group(stored_conn *sconn, modb_ref *modb,
                            unsigned int metadata_id, unsigned int group_id);
int modbUnlink_Metadata_Group(stored_conn *sconn, modb_ref *modb,
                              unsigned int metadata_id, unsigned int group_id);

// MODB Group -> Metadatas
int modbFetchGroupMetadataIds(stored_conn *sconn, modb_ref *modb,
                              unsigned int group_id, int with_deleted,
                              unsigned int **metadata_ids, size_t *n_ids);

#endif // H__MODB_METADATA__
