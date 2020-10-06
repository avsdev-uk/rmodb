#ifndef H__MODB_MANAGE_P__
#define H__MODB_MANAGE_P__

#include <stdint.h>
#include <stddef.h>

#include "database.h"
#include "modb_types.h"


uint64_t createSysTable(stored_conn *sconn, modb_ref *modb);
uint64_t createMetaTable(stored_conn *sconn, modb_ref *modb);
uint64_t createObjectsTable(stored_conn *sconn, modb_ref *modb);
uint64_t createMDOGroupsTable(stored_conn *sconn, modb_ref *modb);

uint64_t createUsersTable(stored_conn *sconn, modb_ref *modb);
uint64_t createGroupsTable(stored_conn *sconn, modb_ref *modb);
uint64_t createUserGroupsTable(stored_conn *sconn, modb_ref *modb);

uint64_t createMetaExtTable(stored_conn *sconn, modb_ref *modb,
                            column_data **col_data, size_t cols);

int tableExists(stored_conn *sconn, modb_ref *modb, const char *suffix, size_t suffix_len);

uint64_t destroyTable(stored_conn *sconn, modb_ref *modb, const char *suffix, size_t suffix_len);

int connectionUseMODB(stored_conn *sconn, modb_ref *modb, int override);
int connectionGetUse(stored_conn *sconn, modb_ref *modb);
void connectionReleaseMODB(stored_conn *sconn);

#endif // H__MODB_P__
