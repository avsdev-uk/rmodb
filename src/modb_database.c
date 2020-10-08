#include "modb_database.h"
#include "modb_p.h"
#include "modb_database_p.h"


int modbUse(stored_conn *sconn, modb_ref *modb, int override)
{
  return connectionUseMODB(sconn, modb, override) == 0;
}
int modbFindUse(stored_conn *sconn, modb_ref *modb)
{
  return connectionGetMODB(sconn, modb) == 1;
}
void modbReleaseUse(stored_conn *sconn)
{
  connectionUnuseMODB(sconn);
}


int modbCreate(stored_conn *sconn, modb_ref *modb, column_data **col_data, size_t n_cols)
{
  int err = (createSysTable(sconn, modb) == (uint64_t)-1)
      || (createMetaTable(sconn, modb) == (uint64_t)-1)
      || (createObjectsTable(sconn, modb) == (uint64_t)-1)
      || (createMDOGroupsTable(sconn, modb) == (uint64_t)-1)
      || (createUsersTable(sconn, modb) == (uint64_t)-1)
      || (createGroupsTable(sconn, modb) == (uint64_t)-1)
      || (createUserGroupsTable(sconn, modb) == (uint64_t)-1);

  if (col_data != 0) {
    err = err || createMetaExtTable(sconn, modb, col_data, n_cols);
  }

  return !err;
}
int modbExists(stored_conn *sconn, modb_ref *modb)
{
  return MODBTableExists(sconn, modb, METADATA_TABLE, STR_LEN(METADATA_TABLE));
}
int modbHasExtendedMetadata(stored_conn *sconn, modb_ref *modb)
{
  return MODBTableExists(sconn, modb, META_EXT_TABLE, STR_LEN(META_EXT_TABLE));
}
int modbDestroy(stored_conn *sconn, modb_ref *modb)
{
  uint64_t err = 0
      | destroyMODBTable(sconn, modb, MDO_GROUPS_TABLE, STR_LEN(MDO_GROUPS_TABLE))
      | destroyMODBTable(sconn, modb, OBJECTS_TABLE, STR_LEN(OBJECTS_TABLE))
      | destroyMODBTable(sconn, modb, METADATA_TABLE, STR_LEN(METADATA_TABLE))
      | destroyMODBTable(sconn, modb, SYS_TABLE, STR_LEN(SYS_TABLE))
      | destroyMODBTable(sconn, modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE))
      | destroyMODBTable(sconn, modb, GROUPS_TABLE, STR_LEN(GROUPS_TABLE))
      | destroyMODBTable(sconn, modb, USERS_TABLE, STR_LEN(USERS_TABLE))
      | destroyMODBTable(sconn, modb, META_EXT_TABLE, STR_LEN(META_EXT_TABLE));
  return err == 0;
}
