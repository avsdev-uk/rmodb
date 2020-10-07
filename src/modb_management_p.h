#ifndef H__MODB_MANAGEMENT_P__
#define H__MODB_MANAGEMENT_P__

#include <stdint.h>
#include <stddef.h>

#include "database.h"
#include "modb_types.h"


int connectionUseMODB(stored_conn *sconn, modb_ref *modb, int override);
int connectionGetMODB(stored_conn *sconn, modb_ref *modb);
void connectionUnuseMODB(stored_conn *sconn);

uint64_t createMODBTable(stored_conn *sconn, modb_ref *modb, const char *suffix, size_t suffix_len,
                         const char *table_def, size_t table_def_len);
int MODBTableExists(stored_conn *sconn, modb_ref *modb, const char *suffix, size_t suffix_len);
uint64_t destroyMODBTable(stored_conn *sconn, modb_ref *modb, const char *suffix, size_t suffix_len);

uint64_t createSysTable(stored_conn *sconn, modb_ref *modb);
uint64_t createMetaTable(stored_conn *sconn, modb_ref *modb);
uint64_t createObjectsTable(stored_conn *sconn, modb_ref *modb);
uint64_t createMDOGroupsTable(stored_conn *sconn, modb_ref *modb);

uint64_t createUsersTable(stored_conn *sconn, modb_ref *modb);
uint64_t createGroupsTable(stored_conn *sconn, modb_ref *modb);
uint64_t createUserGroupsTable(stored_conn *sconn, modb_ref *modb);

uint64_t createMetaExtTable(stored_conn *sconn, modb_ref *modb,
                            column_data **col_data, size_t cols);

#endif // H__MODB_MANAGEMENT_P__
