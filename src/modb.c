#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "modb.h"
#include "modb_p.h"
#include "db_query.h"
#include "strext.h"


int modbCreate(struct stored_conn_t *sconn, struct modb_t *modb)
{
  if (createSysTable(sconn, modb) == (uint64_t)-1) {
    return -1;
  }
  if (createMetaTable(sconn, modb) == (uint64_t)-1) {
    destroyTable(sconn, modb, SYS_TABLE, STR_LEN(SYS_TABLE));
    return -1;
  }
  if (createObjectsTable(sconn, modb) == (uint64_t)-1) {
    destroyTable(sconn, modb, META_TABLE, STR_LEN(META_TABLE));
    destroyTable(sconn, modb, SYS_TABLE, STR_LEN(SYS_TABLE));
    return -1;
  }
  if (createMDOGroupsTable(sconn, modb) == (uint64_t)-1) {
    destroyTable(sconn, modb, OBJECTS_TABLE, STR_LEN(OBJECTS_TABLE));
    destroyTable(sconn, modb, META_TABLE, STR_LEN(META_TABLE));
    destroyTable(sconn, modb, SYS_TABLE, STR_LEN(SYS_TABLE));
    return -1;
  }

  return 0;
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
    return -1;
  }
  if (createGroupsTable(sconn, modb) == (uint64_t)-1) {
    destroyTable(sconn, modb, USERS_TABLE, STR_LEN(USERS_TABLE));
    return -1;
  }
  if (createUserGroupsTable(sconn, modb) == (uint64_t)-1) {
    destroyTable(sconn, modb, GROUPS_TABLE, STR_LEN(GROUPS_TABLE));
    destroyTable(sconn, modb, USERS_TABLE, STR_LEN(USERS_TABLE));
    return -1;
  }

  return 0;
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
  char *qry;
  uint64_t res;
  size_t qry_len;
  str_builder *sb;
  char *colstr;

  if ((sb = strbld_create()) == 0) {
    return -1;
  }
  strbld_str(sb, "CREATE TABLE `", 0);
  strbld_str(sb, modb->name, modb->name_len);
  strbld_str(sb, META_EXT_TABLE, STR_LEN(META_EXT_TABLE));
  strbld_str(sb, "` ", 2);
  strbld_str(sb, "(", 1);
  strbld_str(sb, "`mdo_id` INT UNSIGNED NOT NULL", 0);
  for (size_t c = 0; c < cols; c++) {
    struct column_data_t *col = *(col_data + c);
    if ((colstr = createColString(col)) == 0) {
      strbld_destroy(&sb);
      return -1;
    }
    strbld_str(sb, colstr, 0);
    free(colstr);
  }
  strbld_str(sb, ", INDEX (`mdo_id`))", 0);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return -1;
  }

  res = simpleQuery(sconn, qry, qry_len);
  free(qry);

  return (res == (uint64_t)-1) ? -1 : (int)res;
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
