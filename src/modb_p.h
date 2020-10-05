#ifndef H__MODB_P__
#define H__MODB_P__

#include <stdint.h>
#include <stddef.h>

#include "database.h"
#include "modb_types.h"


#define SYS_TABLE "_sys"
#define META_TABLE "_meta"
#define OBJECTS_TABLE "_objects"
#define MDO_GROUPS_TABLE "_mdo_groups"

#define USERS_TABLE "_users"
#define GROUPS_TABLE "_groups"
#define USER_GROUPS_TABLE "_user_groups"

#define META_EXT_TABLE "_meta_ext"


uint64_t createSysTable(struct stored_conn_t *sconn, struct modb_t *modb);
uint64_t createMetaTable(struct stored_conn_t *sconn, struct modb_t *modb);
uint64_t createObjectsTable(struct stored_conn_t *sconn, struct modb_t *modb);
uint64_t createMDOGroupsTable(struct stored_conn_t *sconn, struct modb_t *modb);

uint64_t createUsersTable(struct stored_conn_t *sconn, struct modb_t *modb);
uint64_t createGroupsTable(struct stored_conn_t *sconn, struct modb_t *modb);
uint64_t createUserGroupsTable(struct stored_conn_t *sconn, struct modb_t *modb);

uint64_t createMetaExtTable(struct stored_conn_t *sconn, struct modb_t *modb,
                            struct column_data_t **col_data, size_t cols);

int tableExists(struct stored_conn_t *sconn, struct modb_t *modb,
                const char *suffix, size_t suffix_len);

uint64_t destroyTable(struct stored_conn_t *sconn, struct modb_t *modb,
                      const char *suffix, size_t suffix_len);

int connectionUseMODB(struct stored_conn_t *sconn, struct modb_t *modb, int override);
int connectionGetUse(struct stored_conn_t *sconn, struct modb_t *modb);
void connectionReleaseMODB(struct stored_conn_t *sconn);

#endif // H__MODB_P__
