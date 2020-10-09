
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "modb_groups.h"
#include "modb_users.h"
#include "modb_p.h"


// ##### PRIVATE
int tableRowsToGroups(column_data **col_data, size_t n_cols,
                      struct group_t ***groups, size_t *n_groups)
{
  column_data *col_id, *col_name, *col_created, *col_updated, *col_deleted;
  size_t n_rows, idx;
  struct group_t *group;

  n_rows = (*col_data)->n_values;
  if (n_rows == 0) {
    return 0;
  }

  col_id = findColumnByName(col_data, n_cols, "id");
  col_name = findColumnByName(col_data, n_cols, "name");
  col_created = findColumnByName(col_data, n_cols, "created");
  col_updated = findColumnByName(col_data, n_cols, "updated");
  col_deleted = findColumnByName(col_data, n_cols, "deleted");

  *groups = (struct group_t **)malloc(sizeof(struct group_t *) * n_rows);
  if (*groups == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return -1;
  }
  memset(*groups, 0, sizeof(struct group_t *) * n_rows);

  for (idx = 0; idx < n_rows; idx++) {
    if ((group = allocGroup()) == 0) {
      freeGroups(groups, idx - 1);
      return -1;
    }

    group->id = *(col_id->data.ptr_uint32 + idx);

    if (!moveColumnStrPointer(col_name, idx, 1, &group->name, &group->name_len)) {
      freeGroups(groups, idx);
      return -1;
    }

    group->created_on = *(col_created->data.ptr_int64 + idx);
    if (!columnRowIsNull(col_updated, idx)) {
      group->updated_on = *(col_updated->data.ptr_int64 + idx);
    }
    if (!columnRowIsNull(col_deleted, idx)) {
      group->deleted_on = *(col_deleted->data.ptr_int64 + idx);
    }

    *(*groups + idx) = group;
  }

  *n_groups = n_rows;

  return 1;
}
int doGroupsQuery(stored_conn *sconn, modb_ref *modb, where_builder *wb,
                  struct group_t ***groups, size_t *n_groups)
{
  char *table;
  size_t table_len;
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;
  int res;

  str_builder *sb;

  column_data **col_data;
  size_t n_cols;


  if ((sb = strbld_create()) == 0) {
    return -1;
  }
  modbTableName(&table, &table_len, modb, GROUPS_TABLE, strlen(GROUPS_TABLE));

  strbld_str(sb, "SELECT * FROM ", 0);
  escapeTableName_sb(sb, table, table_len);
  if (wb != 0) {
    compileWhereBuilder_sb(sb, wb, 0);
  }

  modbFreeTableName(&table);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return -1;
  }

  qry_ret = tableQuery(sconn, qry, qry_len, 0, &col_data, &n_cols);
  free(qry);

  // Query failed
  if (qry_ret == (uint64_t)-1) {
    return -1;
  }

  // Zero row result
  if (qry_ret == 0) {
    freeColumns(col_data, n_cols);
    return 0;
  }

  res = tableRowsToGroups(col_data, n_cols, groups, n_groups);
  freeColumns(col_data, n_cols);

  if (res <= 0) {
    return res;
  }

  return (int)qry_ret;
}
int doScalarGroupsQuery(stored_conn *sconn, modb_ref *modb,
                        where_builder *wb, int with_members, struct group_t **group)
{
  int res;
  struct group_t **groups;
  size_t n_groups;

  res = doGroupsQuery(sconn, modb, wb, &groups, &n_groups);
  if (res <= 0) {
    return res;
  }

  *group = *(groups + 0);
  *(groups + 0) = 0;
  freeGroups(&groups, n_groups);

  if (with_members) {
    if (modbFetchGroupUsers(sconn, modb, *group, 0) < 0) {
      freeGroup(group);
      return -1;
    }
  }

  return res;
}


// ##### PUBLIC
struct group_t *allocGroup(void)
{
  struct group_t *group;

  group = malloc(sizeof(struct group_t));
  if (group == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
  }
  memset(group, 0, sizeof(struct group_t));

  return group;
}
struct group_t **allocGroups(size_t n_groups)
{
  struct group_t **groups;

  groups = (struct group_t **)malloc(sizeof(struct group_t *) * n_groups);
  if (groups == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }
  memset(groups, 0, sizeof(struct group_t *) * n_groups);

  return groups;
}
void freeGroup(struct group_t **group_ptr)
{
  struct group_t *group = *group_ptr;

  if (group->n_members > 0) {
    if (group->member_ids != 0) {
      free(group->member_ids);
      group->member_ids = 0;
    }
    if (group->members != 0) {
      freeUsers(&group->members, group->n_members);
    }
    group->n_members = 0;
  }

  if (group->name != 0) {
    free(group->name);
    group->name = 0;
  }

  free(group);
  *group_ptr = 0;
}
void freeGroups(struct group_t ***groups_ptr, size_t n_groups)
{
  size_t idx;
  struct group_t **groups = *groups_ptr;

  for (idx = 0; idx < n_groups; idx++) {
    if (*(groups + idx) != 0) {
      freeGroup(groups + idx);
    }
  }

  free(groups);
  *groups_ptr = 0;
}


int modbGroupById(stored_conn *sconn, modb_ref *modb, unsigned int id, int with_members,
                  struct group_t **group)
{
  where_builder *wb = 0;
  int res;

  wb = where(0, "id", EQ, TYPE_ID, 1, id);
  res = doScalarGroupsQuery(sconn, modb, wb, with_members, group);
  freeWhereBuilder(&wb);

  return res;
}
int modbGroupByName(stored_conn *sconn, modb_ref *modb, const char *name, int with_members,
                    struct group_t **group)
{
  where_builder *wb = 0;
  int res;

  wb = where(0, "name", EQ, TYPE_STRING, 1, name);
  res = doScalarGroupsQuery(sconn, modb, wb, with_members, group);
  freeWhereBuilder(&wb);

  return res;
}

int modbGroupList(stored_conn *sconn, modb_ref *modb, int with_deleted, int with_members,
                  struct group_t ***groups, size_t *n_groups)
{
  where_builder *wb = 0;
  int res;
  size_t idx;
  struct group_t *group;

  if (with_deleted == 0) {
    wb = where(0, "deleted_on", IS_NULL, TYPE_RAW, 0, 0);
  }
  res = doGroupsQuery(sconn, modb, wb, groups, n_groups);
  if (wb != 0) {
    freeWhereBuilder(&wb);
  }

  if (res > 0 && with_members) {
    for (idx = 0; idx < *n_groups; idx++) {
      group = *((*groups) + idx);
      if (modbFetchGroupUsers(sconn, modb, group, with_deleted) < 0) {
        freeGroups(groups, *n_groups);
        *groups = 0;
        *n_groups = 0;
        return -1;
      }
    }
  }

  return res;
}

int64_t modbGroupCreate(stored_conn *sconn, modb_ref *modb,
                        unsigned int id, const char *name)
{
  str_builder *sb;
  char *table;
  size_t table_len;
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return -1;
  }
  modbTableName(&table, &table_len, modb, GROUPS_TABLE, strlen(GROUPS_TABLE));

  strbld_str(sb, "INSERT INTO ", 0);
  escapeTableName_sb(sb, table, table_len);
  strbld_str(sb, " (`id`, `name`) VALUES (", 0);
  db_value_sb(sb, TYPE_ID, 1, id);
  strbld_char(sb, ',');
  db_value_sb(sb, TYPE_STRING, 1, name);
  strbld_char(sb, ')');

  modbFreeTableName(&table);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return -1;
  }

  qry_ret = simpleQuery(sconn, qry, qry_len);
  free(qry);

  return (int64_t)qry_ret;
}
int64_t modbGroupUpdate(stored_conn *sconn, modb_ref *modb, unsigned int id,
                        const char *name)
{
  str_builder *sb;
  char *table, *set;
  size_t table_len, set_len;
  int64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  columnSetValueStr_sb(sb, "id", TYPE_ID, 1, id);
  if (name != 0) {
    strbld_char(sb, ',');
    columnSetValueStr_sb(sb, "name", TYPE_STRING, 1, name);
  }
  if (strbld_finalize_or_destroy(&sb, &set, &set_len) != 0) {
    return 0;
  }

  modbTableName(&table, &table_len, modb, GROUPS_TABLE, strlen(GROUPS_TABLE));
  qry_ret = updateQuery(sconn, table, table_len, set, set_len, where(0, "id", EQ, TYPE_ID, 1, id));
  modbFreeTableName(&table);

  free(set);

  return qry_ret;
}
int modbGroupDelete(stored_conn *sconn, modb_ref *modb, unsigned int id)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, GROUPS_TABLE, STR_LEN(GROUPS_TABLE));
  qry_ret = softDeleteByIdQuery(sconn, table, table_len, "id", id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}
int modbGroupDestroy(stored_conn *sconn, modb_ref *modb, unsigned int id)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, GROUPS_TABLE, STR_LEN(GROUPS_TABLE));
  qry_ret = deleteByIdQuery(sconn, table, table_len, "id", id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}


int modbFetchGroupUserIds(stored_conn *sconn, modb_ref *modb, struct group_t *group,
                          int with_deleted)
{
  char *u_table, *ug_table;
  size_t u_len, ug_len;

  char *qry;
  size_t qry_len;
  uint64_t qry_ret;

  str_builder *sb;
  where_builder *wb;

  column_data **col_data;
  size_t n_cols;


  if ((sb = strbld_create()) == 0) {
    return -1;
  }
  modbTableName(&u_table, &u_len, modb, USERS_TABLE, STR_LEN(USERS_TABLE));
  modbTableName(&ug_table, &ug_len, modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE));

  strbld_str(sb, "SELECT ", 0);
  escapeColumnName_sb(sb, 0, 0, "user_id", 0);
  strbld_str(sb, " FROM ", 0);
  escapeTableName_sb(sb, ug_table, ug_len);
  joinStr_sb(sb, " LEFT", 5, 1, u_table, u_len, "id", 2, ug_table, ug_len, "user_id", 8);
  wb = where(0, "group_id", EQ, TYPE_ID, 1, group->id);
  if (!with_deleted) {
    wb = whereAnd(wb, where(u_table, "deleted", IS_NULL, TYPE_RAW, 0));
  }
  compileWhereBuilder_sb(sb, wb, 1);

  modbFreeTableName(&u_table);
  modbFreeTableName(&ug_table);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return -1;
  }

  qry_ret = tableQuery(sconn, qry, qry_len, 0, &col_data, &n_cols);
  free(qry);

  // Query failed
  if (qry_ret == (uint64_t)-1) {
    return -1;
  }

  // Zero row result
  if (qry_ret == 0) {
    freeColumns(col_data, n_cols);
    return 0;
  }

  group->member_ids = (*col_data)->data.ptr_uint32;
  group->n_members = (*col_data)->n_values;
  (*col_data)->data.ptr_uint32 = 0;

  freeColumns(col_data, n_cols);

  return 1;
}
int modbFetchGroupUsers(stored_conn *sconn, modb_ref *modb, struct group_t *group,
                        int with_deleted)
{
  int ret;
  size_t idx;

  ret = modbFetchGroupUserIds(sconn, modb, group, with_deleted);
  if (ret != 1) {
    return ret;
  }

  group->members = allocUsers(group->n_members);
  if (group->members == 0) {
    ret = -1;
  }

  for (idx = 0; idx < group->n_members; idx++) {
    if (modbUserById(sconn, modb, *(group->member_ids + idx), 0, (group->members + idx)) != 1) {
      freeUsers(&group->members, group->n_members);
      group->members = 0;
      ret = -1;
    }
  }

  return ret;
}



// ##### USER<->GROUP RELATIONS
int modbSyncGroupUsers(stored_conn *sconn, modb_ref *modb,
                       unsigned int group_id, size_t n_users, unsigned int *user_ids)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE));
  qry_ret = syncIdMap(sconn, table, table_len, "group_id", "user_id", group_id, n_users, user_ids);
  modbFreeTableName(&table);

  return (int)qry_ret;
}
int modbSyncGroupUsers_va(stored_conn *sconn, modb_ref *modb,
                          unsigned int group_id, size_t n_users, ...)
{
  va_list args;
  char *table;
  size_t table_len;
  int64_t qry_ret;

  va_start(args, n_users);
  modbTableName(&table, &table_len, modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE));
  qry_ret = syncIdMap_va(sconn, table, table_len, "group_id", "user_id", group_id, n_users, args);
  modbFreeTableName(&table);
  va_end(args);

  return (int)qry_ret;
}

int modbIsLinked_Group_User(stored_conn *sconn, modb_ref *modb,
                            unsigned int group_id, unsigned int user_id)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE));
  qry_ret = hasIdMap(sconn, table, table_len, "group_id", "user_id", group_id, user_id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}
int modbLink_Group_User(stored_conn *sconn, modb_ref *modb,
                        unsigned int group_id, unsigned int user_id)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE));
  qry_ret = addIdMap(sconn, table, table_len, "group_id", "user_id", group_id, user_id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}
int modbUnlink_Group_User(stored_conn *sconn, modb_ref *modb,
                          unsigned int group_id, unsigned int user_id)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE));
  qry_ret = removeIdMap(sconn, table, table_len, "group_id", "user_id", group_id, user_id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}
