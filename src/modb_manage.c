#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "modb_manage.h"
#include "strext.h"

#include "modb_p.h"
#include "modb_manage_p.h"


int modbUse(stored_conn *sconn, modb_ref *modb, int override)
{
  return connectionUseMODB(sconn, modb, override) == 0;
}
int modbFindUse(stored_conn *sconn, modb_ref *modb)
{
  return connectionGetUse(sconn, modb) == 1;
}
void modbReleaseUse(stored_conn *sconn)
{
  connectionReleaseMODB(sconn);
}


int modbCreate(stored_conn *sconn, modb_ref *modb)
{
  if (createSysTable(sconn, modb) == (uint64_t)-1) {
    return 0;
  }
  if (createMetaTable(sconn, modb) == (uint64_t)-1) {
    destroyTable(sconn, modb, SYS_TABLE, STR_LEN(SYS_TABLE));
    return 0;
  }
  if (createObjectsTable(sconn, modb) == (uint64_t)-1) {
    destroyTable(sconn, modb, META_TABLE, STR_LEN(META_TABLE));
    destroyTable(sconn, modb, SYS_TABLE, STR_LEN(SYS_TABLE));
    return 0;
  }
  if (createMDOGroupsTable(sconn, modb) == (uint64_t)-1) {
    destroyTable(sconn, modb, OBJECTS_TABLE, STR_LEN(OBJECTS_TABLE));
    destroyTable(sconn, modb, META_TABLE, STR_LEN(META_TABLE));
    destroyTable(sconn, modb, SYS_TABLE, STR_LEN(SYS_TABLE));
    return 0;
  }

  return 1;
}
int modbExists(stored_conn *sconn, modb_ref *modb)
{
  return tableExists(sconn, modb, META_TABLE, STR_LEN(META_TABLE));
}
int modbDestroy(stored_conn *sconn, modb_ref *modb)
{
  uint64_t err = 0
      | destroyTable(sconn, modb, MDO_GROUPS_TABLE, STR_LEN(MDO_GROUPS_TABLE))
      | destroyTable(sconn, modb, OBJECTS_TABLE, STR_LEN(OBJECTS_TABLE))
      | destroyTable(sconn, modb, META_TABLE, STR_LEN(META_TABLE))
      | destroyTable(sconn, modb, SYS_TABLE, STR_LEN(SYS_TABLE));
  return err == 0;
}

int modbAccountingCreate(stored_conn *sconn, modb_ref *modb)
{
  if (createUsersTable(sconn, modb) == (uint64_t)-1) {
    return 0;
  }
  if (createGroupsTable(sconn, modb) == (uint64_t)-1) {
    destroyTable(sconn, modb, USERS_TABLE, STR_LEN(USERS_TABLE));
    return 0;
  }
  if (createUserGroupsTable(sconn, modb) == (uint64_t)-1) {
    destroyTable(sconn, modb, GROUPS_TABLE, STR_LEN(GROUPS_TABLE));
    destroyTable(sconn, modb, USERS_TABLE, STR_LEN(USERS_TABLE));
    return 0;
  }

  return 1;
}
int modbAccountingExists(stored_conn *sconn, modb_ref *modb)
{
  return tableExists(sconn, modb, USERS_TABLE, STR_LEN(USERS_TABLE));
}
int modbAccountingDestroy(stored_conn *sconn, modb_ref *modb)
{
  uint64_t err = 0
      | destroyTable(sconn, modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE))
      | destroyTable(sconn, modb, GROUPS_TABLE, STR_LEN(GROUPS_TABLE))
      | destroyTable(sconn, modb, USERS_TABLE, STR_LEN(USERS_TABLE));
  return err == 0;
}

int modbMetaExtCreate(stored_conn *sconn, modb_ref *modb,
                      column_data **col_data, size_t cols)
{
  uint64_t err = 0
      | createMetaExtTable(sconn, modb, col_data, cols);
  return err == 0;
}
int modbMetaExtExists(stored_conn *sconn, modb_ref *modb)
{
  return tableExists(sconn, modb, META_EXT_TABLE, STR_LEN(META_EXT_TABLE));
}
int modbMetaExtDestroy(stored_conn *sconn, modb_ref *modb)
{
  uint64_t err = 0
      | destroyTable(sconn, modb, META_EXT_TABLE, STR_LEN(META_EXT_TABLE));
  return err == 0;
}
