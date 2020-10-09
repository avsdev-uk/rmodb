#include <string.h>
#include <errno.h>

#include "modb_accounting.h"
#include "modb_p.h"
#include "modb_accounting_p.h"


// ##### USERS
int modbUserById(stored_conn *sconn, modb_ref *modb, unsigned int id, int with_groups,
                 struct user_t **user)
{
  where_builder *wb = 0;
  int res;

  wb = where(0, "id", EQ, TYPE_UINT32, 1, id);
  res = doScalarUsersQuery(sconn, modb, wb, with_groups, user);
  freeWhereBuilder(&wb);

  return res;
}
int modbUserByName(stored_conn *sconn, modb_ref *modb, const char *username, int with_groups,
                   struct user_t **user)
{
  where_builder *wb = 0;
  int res;

  wb = where(0, "username", EQ, TYPE_STRING, 1, username);
  res = doScalarUsersQuery(sconn, modb, wb, with_groups, user);
  freeWhereBuilder(&wb);

  return res;
}
int modbUserByEmail(stored_conn *sconn, modb_ref *modb, const char *email, int with_groups,
                   struct user_t **user)
{
  where_builder *wb = 0;
  int res;

  wb = where(0, "email", EQ, TYPE_STRING, 1, email);
  res = doScalarUsersQuery(sconn, modb, wb, with_groups, user);
  freeWhereBuilder(&wb);

  return res;
}
int modbUserSearch(stored_conn *sconn, modb_ref *modb, const char *username_email, int with_groups,
                   struct user_t **user)
{
  where_builder *wb = 0;
  int res;

  wb = whereOr(
        where(0, "username", EQ, TYPE_STRING, 1, username_email),
        where(0, "email", EQ, TYPE_STRING, 1, username_email)
        );
  res = doScalarUsersQuery(sconn, modb, wb, with_groups, user);
  freeWhereBuilder(&wb);

  return res;
}

int modbUserList(stored_conn *sconn, modb_ref *modb, int with_deleted, int with_groups,
                 struct user_t ***users, size_t *n_users)
{
  where_builder *wb = 0;
  int res;
  size_t idx;
  struct user_t *user;

  if (with_deleted == 0) {
    wb = where(0, "deleted_on", IS_NULL, TYPE_RAW, 0, 0);
  }
  res = doUsersQuery(sconn, modb, wb, users, n_users);
  if (wb != 0) {
    freeWhereBuilder(&wb);
  }

  if (res > 0 && with_groups) {
    for (idx = 0; idx < *n_users; idx++) {
      user = *((*users) + idx);
      if (modbFetchUserGroups(sconn, modb, user, with_deleted) < 0) {
        freeUsers(users, *n_users);
        return -1;
      }
    }
  }

  return res;
}

int64_t modbUserCreate(stored_conn *sconn, modb_ref *modb,
                       unsigned int id, const char *user_name, const char *email)
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
  modbTableName(&table, &table_len, modb, USERS_TABLE, strlen(USERS_TABLE));

  strbld_str(sb, "INSERT INTO ", 0);
  escapeTableName_sb(sb, table, table_len);
  strbld_str(sb, " (`id`, `username`, `email`) VALUES (", 0);
  db_value_sb(sb, TYPE_UINT32, 1, id);
  strbld_char(sb, ',');
  db_value_sb(sb, TYPE_STRING, 1, user_name);
  strbld_char(sb, ',');
  db_value_sb(sb, TYPE_STRING, 1, email);
  strbld_char(sb, ')');

  modbFreeTableName(&table);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return -1;
  }

  qry_ret = simpleQuery(sconn, qry, qry_len);
  free(qry);

  return (int64_t)qry_ret;
}
int64_t modbUserUpdate(stored_conn *sconn, modb_ref *modb, unsigned int id,
                       const char *username, const char *email)
{
  str_builder *sb;
  char *table, *set;
  size_t table_len, set_len;
  int64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  columnSetValueStr_sb(sb, "id", TYPE_ID, 1, id);
  if (username != 0) {
    strbld_char(sb, ',');
    columnSetValueStr_sb(sb, "username", TYPE_STRING, 1, username);
  }
  if (email != 0) {
    strbld_char(sb, ',');
    columnSetValueStr_sb(sb, "email", TYPE_STRING, 1, email);
  }
  if (strbld_finalize_or_destroy(&sb, &set, &set_len) != 0) {
    return 0;
  }

  modbTableName(&table, &table_len, modb, USERS_TABLE, strlen(USERS_TABLE));
  qry_ret = updateQuery(sconn, table, table_len, set, set_len, where(0, "id", EQ, TYPE_ID, 1, id));
  modbFreeTableName(&table);

  free(set);

  return qry_ret;
}
int modbUserDelete(stored_conn *sconn, modb_ref *modb, unsigned int id)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, USERS_TABLE, STR_LEN(USERS_TABLE));
  qry_ret = softDeleteByIdQuery(sconn, table, table_len, "id", id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}
int modbUserDestroy(stored_conn *sconn, modb_ref *modb, unsigned int id)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, USERS_TABLE, STR_LEN(USERS_TABLE));
  qry_ret = deleteByIdQuery(sconn, table, table_len, "id", id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}


int modbFetchUserGroupIds(stored_conn *sconn, modb_ref *modb,
                          struct user_t *user, int with_deleted)
{
  char *g_table, *ug_table;
  size_t g_len, ug_len;
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
  modbTableName(&g_table, &g_len, modb, GROUPS_TABLE, STR_LEN(GROUPS_TABLE));
  modbTableName(&ug_table, &ug_len, modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE));

  strbld_str(sb, "SELECT ", 0);
  escapeColumnName_sb(sb, 0, 0, "group_id", 0);
  strbld_str(sb, " FROM ", 0);
  escapeTableName_sb(sb, ug_table, ug_len);
  joinStr_sb(sb, " LEFT", 5, 1, g_table, g_len, "id", 2, ug_table, ug_len, "group_id", 8);
  wb = where(0, "user_id", EQ, TYPE_ID, 1, user->id);
  if (!with_deleted) {
    wb = whereAnd(wb, where(g_table, "deleted", IS_NULL, TYPE_RAW, 0));
  }
  compileWhereBuilder_sb(sb, wb, 1);

  modbFreeTableName(&g_table);
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

  user->group_ids = (*col_data)->data.ptr_uint32;
  user->n_groups = (*col_data)->n_values;
  (*col_data)->data.ptr_uint32 = 0;

  freeColumns(col_data, n_cols);

  return 1;
}
int modbFetchUserGroups(stored_conn *sconn, modb_ref *modb, struct user_t *user, int with_deleted)
{
  int ret;
  size_t idx;

  ret = modbFetchUserGroupIds(sconn, modb, user, with_deleted);
  if (ret != 1) {
    return ret;
  }

  user->groups = allocGroups(user->n_groups);
  if (user->groups == 0) {
    return -1;
  }

  for (idx = 0; idx < user->n_groups; idx++) {
    if (modbGroupById(sconn, modb, *(user->group_ids + idx), 0, (user->groups + idx)) != 1) {
      freeGroups(&user->groups, user->n_groups);
      return -1;
    }
  }

  return ret;
}


int modbSyncUserGroups(stored_conn *sconn, modb_ref *modb,
                       unsigned int user_id, size_t n_groups, unsigned int *group_ids)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE));
  qry_ret = syncIdMap(sconn, table, table_len, "user_id", "group_id", user_id, n_groups, group_ids);
  modbFreeTableName(&table);

  return (int)qry_ret;
}
int modbSyncUserGroups_va(stored_conn *sconn, modb_ref *modb,
                          unsigned int user_id, size_t n_groups, ...)
{
  va_list args;
  char *table;
  size_t table_len;
  int64_t qry_ret;

  va_start(args, n_groups);
  modbTableName(&table, &table_len, modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE));
  qry_ret = syncIdMap_va(sconn, table, table_len, "user_id", "group_id", user_id, n_groups, args);
  modbFreeTableName(&table);
  va_end(args);

  return (int)qry_ret;
}

int modbIsLinked_User_Group(stored_conn *sconn, modb_ref *modb,
                            unsigned int user_id, unsigned int group_id)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE));
  qry_ret = hasIdMap(sconn, table, table_len, "user_id", "group_id", user_id, group_id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}
int modbLink_User_Group(stored_conn *sconn, modb_ref *modb,
                        unsigned int user_id, unsigned int group_id)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE));
  qry_ret = hasIdMap(sconn, table, table_len, "user_id", "group_id", user_id, group_id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}
int modbUnlink_User_Group(stored_conn *sconn, modb_ref *modb,
                          unsigned int user_id, unsigned int group_id)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE));
  qry_ret = hasIdMap(sconn, table, table_len, "user_id", "group_id", user_id, group_id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}



// ##### GROUPS
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
  qry_ret = addIdMap(sconn, table, table_len, "group_id", "user_id", group_id, user_id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}
