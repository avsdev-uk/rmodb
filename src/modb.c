#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "modb.h"
#include "strext.h"

#include "modb_p.h"


int modbUse(struct stored_conn_t *sconn, struct modb_t *modb, int override)
{
  return connectionUseMODB(sconn, modb, override) == 0;
}
int modbFindUse(struct stored_conn_t *sconn, struct modb_t *modb)
{
  return connectionGetUse(sconn, modb) == 1;
}
void modbReleaseUse(struct stored_conn_t *sconn)
{
  connectionReleaseMODB(sconn);
}


int modbCreate(struct stored_conn_t *sconn, struct modb_t *modb)
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
int modbExists(struct stored_conn_t *sconn, struct modb_t *modb)
{
  return tableExists(sconn, modb, META_TABLE, STR_LEN(META_TABLE));
}
int modbDestroy(struct stored_conn_t *sconn, struct modb_t *modb)
{
  uint64_t err = 0
      | destroyTable(sconn, modb, MDO_GROUPS_TABLE, STR_LEN(MDO_GROUPS_TABLE))
      | destroyTable(sconn, modb, OBJECTS_TABLE, STR_LEN(OBJECTS_TABLE))
      | destroyTable(sconn, modb, META_TABLE, STR_LEN(META_TABLE))
      | destroyTable(sconn, modb, SYS_TABLE, STR_LEN(SYS_TABLE));
  return err == 0;
}

int modbAccountingCreate(struct stored_conn_t *sconn, struct modb_t *modb)
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
int modbAccountingExists(struct stored_conn_t *sconn, struct modb_t *modb)
{
  return tableExists(sconn, modb, USERS_TABLE, STR_LEN(USERS_TABLE));
}
int modbAccountingDestroy(struct stored_conn_t *sconn, struct modb_t *modb)
{
  uint64_t err = 0
      | destroyTable(sconn, modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE))
      | destroyTable(sconn, modb, GROUPS_TABLE, STR_LEN(GROUPS_TABLE))
      | destroyTable(sconn, modb, USERS_TABLE, STR_LEN(USERS_TABLE));
  return err == 0;
}

int modbMetaExtCreate(struct stored_conn_t *sconn, struct modb_t *modb,
                      struct column_data_t **col_data, size_t cols)
{
  uint64_t err = 0
      | createMetaExtTable(sconn, modb, col_data, cols);
  return err == 0;
}
int modbMetaExtExists(struct stored_conn_t *sconn, struct modb_t *modb)
{
  return tableExists(sconn, modb, META_EXT_TABLE, STR_LEN(META_EXT_TABLE));
}
int modbMetaExtDestroy(struct stored_conn_t *sconn, struct modb_t *modb)
{
  uint64_t err = 0
      | destroyTable(sconn, modb, META_EXT_TABLE, STR_LEN(META_EXT_TABLE));
  return err == 0;
}
