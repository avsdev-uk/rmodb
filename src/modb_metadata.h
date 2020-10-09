#ifndef H__MODB_METADATA__
#define H__MODB_METADATA__

#include "database.h"
#include "modb_types.h"

// MODB MetadataList
int modbMetadataById(stored_conn *sconn, modb_ref *modb, unsigned int id,
                     struct metadata_t **metadata);
// MetadataList by owner
// MetadataList by group
// MetadataList by owner/owner groups

int modbMetadataList(stored_conn *sconn, modb_ref *modb, int with_deleted,
                     struct metadata_t ***metadata_list, size_t *n_metadata_list);

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

#endif // H__MODB_METADATA__
