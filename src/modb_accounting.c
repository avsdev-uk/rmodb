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
  res = doScalarUsersQuery(sconn, modb, wb, user);
  freeWhereBuilder(&wb);

  if (with_groups) {
    if (modbFetchUserGroups(
          sconn, modb, 0, (*user)->id, &(*user)->n_groups, &(*user)->groups
          ) < 0) {
      freeUser(user);
      return -1;
    }
  }

  return res;
}
int modbUserByName(stored_conn *sconn, modb_ref *modb, const char *username, int with_groups,
                   struct user_t **user)
{
  where_builder *wb = 0;
  int res;

  wb = where(0, "username", EQ, TYPE_STRING, 1, username);
  res = doScalarUsersQuery(sconn, modb, wb, user);
  freeWhereBuilder(&wb);

  if (with_groups) {
    if (modbFetchUserGroups(
          sconn, modb, 0, (*user)->id, &(*user)->n_groups, &(*user)->groups
          ) < 0) {
      freeUser(user);
      return -1;
    }
  }

  return res;
}
int modbUserByEmail(stored_conn *sconn, modb_ref *modb, const char *email, int with_groups,
                   struct user_t **user)
{
  where_builder *wb = 0;
  int res;

  wb = where(0, "email", EQ, TYPE_STRING, 1, email);
  res = doScalarUsersQuery(sconn, modb, wb, user);
  freeWhereBuilder(&wb);

  if (with_groups) {
    if (modbFetchUserGroups(
          sconn, modb, 0, (*user)->id, &(*user)->n_groups, &(*user)->groups
          ) < 0) {
      freeUser(user);
      return -1;
    }
  }

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
  res = doScalarUsersQuery(sconn, modb, wb, user);
  freeWhereBuilder(&wb);

  if (with_groups) {
    if (modbFetchUserGroups(
          sconn, modb, 0, (*user)->id, &(*user)->n_groups, &(*user)->groups
          ) < 0) {
      freeUser(user);
      return -1;
    }
  }

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

  if (with_groups) {
    for (idx = 0; idx < *n_users; idx++) {
      user = *((*users) + idx);
      if (modbFetchUserGroups(
            sconn, modb, with_deleted, user->id, &user->n_groups, &user->groups
            ) < 0) {
        freeUsers(*users, *n_users);
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
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return -1;
  }
  strbld_str(sb, "INSERT INTO ", 0);
  modbTableName_sb(sb, modb, USERS_TABLE, strlen(USERS_TABLE), '`');
  strbld_str(sb, " (`id`, `username`, `email`) VALUES(", 0);
  db_value_sb(sb, TYPE_UINT32, 1, id);
  strbld_str(sb, ", ", 0);
  db_value_sb(sb, TYPE_STRING, 1, user_name);
  strbld_str(sb, ", ", 0);
  db_value_sb(sb, TYPE_STRING, 1, email);
  strbld_char(sb, ')');
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
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  strbld_str(sb, "UPDATE ", 0);
  modbTableName_sb(sb, modb, USERS_TABLE, strlen(USERS_TABLE), '`');
  strbld_str(sb, " SET `id` = ", 0);
  db_value_sb(sb, TYPE_UINT32, 1, id);

  if (username != 0) {
    strbld_str(sb, ", `username` = ", 0);
    db_value_sb(sb, TYPE_STRING, 1, username);
  }
  if (email != 0) {
    strbld_str(sb, "`email` = ", 0);
    db_value_sb(sb, TYPE_STRING, 1, email);
  }

  strbld_str(sb, " WHERE `id` = ", 0);
  db_value_sb(sb, TYPE_UINT32, 1, id);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return 0;
  }

  qry_ret = simpleQuery(sconn, qry, qry_len);
  free(qry);

  // Query failed
  if (qry_ret == (uint64_t)-1) {
    return 0;
  }

  return 1;
}
int modbUserDelete(stored_conn *sconn, modb_ref *modb, int id)
{
  str_builder *sb;
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  strbld_str(sb, "UPDATE`", 0);
  modbTableName_sb(sb, modb, USERS_TABLE, strlen(USERS_TABLE), '`');
  strbld_str(sb, " SET `deleted` = CURRENT_TIMESTAMP() WHERE `id` = ", 0);
  db_value_sb(sb, TYPE_UINT32, 1, id);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return 0;
  }

  qry_ret = simpleQuery(sconn, qry, qry_len);
  free(qry);

  // Query failed
  if (qry_ret == (uint64_t)-1) {
    return 0;
  }

  return 1;
}
int modbUserDestroy(stored_conn *sconn, modb_ref *modb, int id)
{
  str_builder *sb;
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  strbld_str(sb, "DELETE FROM ", 0);
  modbTableName_sb(sb, modb, USERS_TABLE, strlen(USERS_TABLE), '`');
  strbld_str(sb, " WHERE `id` = ", 0);
  db_value_sb(sb, TYPE_UINT32, 1, id);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return 0;
  }

  qry_ret = simpleQuery(sconn, qry, qry_len);
  free(qry);

  // Query failed
  if (qry_ret == (uint64_t)-1) {
    return 0;
  }

  return 1;
}



// ##### GROUPS
int modbGroupById(stored_conn *sconn, modb_ref *modb, unsigned int id, int with_members,
                  struct group_t **group)
{
  where_builder *wb = 0;
  int res;

  wb = where(0, "id", EQ, TYPE_UINT32, 1, id);
  res = doScalarGroupsQuery(sconn, modb, wb, group);
  freeWhereBuilder(&wb);

  if (with_members) {
    if (modbFetchGroupUsers(
          sconn, modb, 0, (*group)->id, &(*group)->n_members, &(*group)->members
          ) < 0) {
      freeGroup(group);
      return -1;
    }
  }

  return res;
}
int modbGroupByName(stored_conn *sconn, modb_ref *modb, const char *name, int with_members,
                    struct group_t **group)
{
  where_builder *wb = 0;
  int res;

  wb = where(0, "name", EQ, TYPE_STRING, 1, name);
  res = doScalarGroupsQuery(sconn, modb, wb, group);
  freeWhereBuilder(&wb);

  if (with_members) {
    if (modbFetchGroupUsers(
          sconn, modb, 0, (*group)->id, &(*group)->n_members, &(*group)->members
          ) < 0) {
      freeGroup(group);
      return -1;
    }
  }

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

  if (with_members) {
    for (idx = 0; idx < *n_groups; idx++) {
      group = *((*groups) + idx);
      if (modbFetchGroupUsers(
            sconn, modb, with_deleted, group->id, &group->n_members, &group->members
            ) < 0) {
        freeGroups(*groups, *n_groups);
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
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return -1;
  }
  strbld_str(sb, "INSERT INTO`", 0);
  modbTableName_sb(sb, modb, GROUPS_TABLE, strlen(GROUPS_TABLE), '`');
  strbld_str(sb, " (`id`, `name`) VALUES(", 0);
  db_value_sb(sb, TYPE_UINT32, 1, id);
  strbld_str(sb, ", ", 0);
  db_value_sb(sb, TYPE_STRING, 1, name);
  strbld_char(sb, ')');
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
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  strbld_str(sb, "UPDATE ", 0);
  modbTableName_sb(sb, modb, GROUPS_TABLE, strlen(GROUPS_TABLE), '`');
  strbld_str(sb, " SET `id` = ", 0);
  db_value_sb(sb, TYPE_UINT32, 1, id);

  if (name == 0) {
    strbld_str(sb, ", `name` = ", 0);
    db_value_sb(sb, TYPE_STRING, 1, name);
  }

  strbld_str(sb, " WHERE ", 0);
  strbld_str(sb, "`id` = ", 0);
  db_value_sb(sb, TYPE_UINT32, 1, id);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return 0;
  }

  qry_ret = simpleQuery(sconn, qry, qry_len);
  free(qry);

  // Query failed
  if (qry_ret == (uint64_t)-1) {
    return 0;
  }

  return 1;
}
int modbGroupDelete(stored_conn *sconn, modb_ref *modb, int id)
{
  str_builder *sb;
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  strbld_str(sb, "UPDATE ", 0);
  modbTableName_sb(sb, modb, GROUPS_TABLE, strlen(GROUPS_TABLE), '`');
  strbld_str(sb, " SET `deleted` = CURRENT_TIMESTAMP() WHERE `id` = ", 0);
  db_value_sb(sb, TYPE_UINT32, 1, id);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return 0;
  }

  qry_ret = simpleQuery(sconn, qry, qry_len);
  free(qry);

  // Query failed
  if (qry_ret == (uint64_t)-1) {
    return 0;
  }

  return 1;
}
int modbGroupDestroy(stored_conn *sconn, modb_ref *modb, int id)
{
  str_builder *sb;
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  strbld_str(sb, "DELETE FROM ", 0);
  modbTableName_sb(sb, modb, GROUPS_TABLE, strlen(GROUPS_TABLE), '`');
  strbld_str(sb, " WHERE `id` = ", 0);
  db_value_sb(sb, TYPE_UINT32, 1, id);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return 0;
  }

  qry_ret = simpleQuery(sconn, qry, qry_len);
  free(qry);

  // Query failed
  if (qry_ret == (uint64_t)-1) {
    return 0;
  }

  return 1;
}



// ##### USER<->GROUP RELATIONS
int modbSyncGroupsUser(stored_conn *sconn, modb_ref *modb,
                       unsigned int user_id, size_t n_groups, unsigned int *group_ids)
{
  int qry_ret;
  char *table;

  if ((table = modbTableName(modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE), 0)) == 0) {
    return 0;
  }

  qry_ret = syncIdMap(sconn, table, "user_id", "group_id", user_id, n_groups, group_ids);
  free(table);

  return qry_ret;
}
int modbSyncUserGroups_va(stored_conn *sconn, modb_ref *modb,
                          unsigned int user_id, size_t n_groups, ...)
{
  va_list args;
  int qry_ret;
  char *table;

  if ((table = modbTableName(modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE), 0)) == 0) {
    return 0;
  }

  va_start(args, n_groups);
  qry_ret = syncIdMap_va(sconn, table, "user_id", "group_id", user_id, n_groups, args);
  va_end(args);

  free(table);

  return qry_ret;
}
int modbSyncGroupUsers(stored_conn *sconn, modb_ref *modb,
                       unsigned int group_id, size_t n_users, unsigned int *user_ids)
{
  int qry_ret;
  char *table;

  if ((table = modbTableName(modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE), 0)) == 0) {
    return 0;
  }

  qry_ret = syncIdMap(sconn, table, "group_id", "user_id", group_id, n_users, user_ids);
  free(table);

  return qry_ret;
}
int modbSyncGroupUsers_va(stored_conn *sconn, modb_ref *modb,
                          unsigned int group_id, size_t n_users, ...)
{
  va_list args;
  int qry_ret;
  char *table;

  if ((table = modbTableName(modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE), 0)) == 0) {
    return 0;
  }

  va_start(args, n_users);
  qry_ret = syncIdMap_va(sconn, table, "group_id", "user_id", group_id, n_users, args);
  va_end(args);

  free(table);

  return qry_ret;
}


int modbFetchUserGroupIds(stored_conn *sconn, modb_ref *modb, int with_deleted,
                        unsigned int user_id, size_t *n_groups, unsigned int **group_ids)
{
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;

  str_builder *sb;
  where_builder *wb;
  char *table;

  column_data **col_data;
  size_t n_cols;


  if ((sb = strbld_create()) == 0) {
    return -errno;
  }
  strbld_str(sb, "SELECT `group_id` FROM ", 0);
  modbTableName_sb(sb, modb, USER_GROUPS_TABLE, strlen(USER_GROUPS_TABLE), '`');
  strbld_char(sb, ' ');
  modbJoin_sb(sb, modb, "LEFT", 4, 1,
              GROUPS_TABLE, strlen(GROUPS_TABLE), "id", 2,
              USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE), "group_id", 8);
  strbld_str(sb, " WHERE ", 0);
  wb = where(0, "user_id", EQ, TYPE_ID, 1, user_id);
  if (!with_deleted) {
    table = modbTableName(modb, GROUPS_TABLE, STR_LEN(GROUPS_TABLE), 0);
    wb = whereAnd(wb, where(table, "deleted", IS_NULL, TYPE_RAW, 0));
    free(table);
  }
  compileWhereBuilder_sb(wb, sb, 1);
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

  *group_ids = (*col_data)->data.ptr_uint32;
  *n_groups = (*col_data)->n_values;
  (*col_data)->data.ptr_uint32 = 0;

  freeColumns(col_data, n_cols);

  return 1;
}
int modbFetchGroupUserIds(stored_conn *sconn, modb_ref *modb, int with_deleted,
                        unsigned int group_id, size_t *n_users, unsigned int **user_ids)
{
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;

  str_builder *sb;
  where_builder *wb;
  char *table;

  column_data **col_data;
  size_t n_cols;


  if ((sb = strbld_create()) == 0) {
    return -errno;
  }
  strbld_str(sb, "SELECT `user_id` FROM ", 0);
  modbTableName_sb(sb, modb, USER_GROUPS_TABLE, strlen(USER_GROUPS_TABLE), '`');
  strbld_char(sb, ' ');
  modbJoin_sb(sb, modb, "LEFT", 4, 1,
              USERS_TABLE, strlen(USERS_TABLE), "id", 2,
              USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE), "user_id", 8);
  strbld_str(sb, " WHERE ", 0);
  wb = where(0, "group_id", EQ, TYPE_ID, 1, group_id);
  if (!with_deleted) {
    table = modbTableName(modb, USERS_TABLE, STR_LEN(USERS_TABLE), 0);
    wb = whereAnd(wb, where(table, "deleted", IS_NULL, TYPE_RAW, 0));
    free(table);
  }
  compileWhereBuilder_sb(wb, sb, 1);
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

  *user_ids = (*col_data)->data.ptr_uint32;
  *n_users = (*col_data)->n_values;
  (*col_data)->data.ptr_uint32 = 0;

  freeColumns(col_data, n_cols);

  return 1;
}
int modbFetchUserGroups(stored_conn *sconn, modb_ref *modb, int with_deleted,
                        unsigned int user_id, size_t *n_groups, struct group_t ***groups)
{
  unsigned int *group_ids;
  int ret;
  size_t idx;

  ret = modbFetchUserGroupIds(sconn, modb, with_deleted, user_id, n_groups, &group_ids);
  if (ret != 1) {
    return ret;
  }

  *groups = allocGroups(*n_groups);
  if (*groups == 0) {
    ret = -1;
  }

  for (idx = 0; idx < *n_groups; idx++) {
    if (modbGroupById(sconn, modb, *(group_ids + idx), 0, (*groups) + idx) != 1) {
      freeGroups(*groups, *n_groups);
      *groups = 0;
      *n_groups = 0;
      ret = -1;
    }
  }
  free(group_ids);

  return ret;
}
int modbFetchGroupUsers(stored_conn *sconn, modb_ref *modb, int with_deleted,
                        unsigned int group_id, size_t *n_users, struct user_t ***users)
{
  unsigned int *user_ids;
  int ret;
  size_t idx;

  ret = modbFetchGroupUserIds(sconn, modb, with_deleted, group_id, n_users, &user_ids);
  if (ret != 1) {
    return ret;
  }

  *users = allocUsers(*n_users);
  if (*users == 0) {
    ret = -1;
  }

  for (idx = 0; idx < *n_users; idx++) {
    if (modbUserById(sconn, modb, *(user_ids + idx), 0, (*users) + idx) != 1) {
      freeUsers(*users, *n_users);
      *users = 0;
      *n_users = 0;
      ret = -1;
    }
  }
  free(user_ids);

  return ret;
}


int modbIsLinked_Group_User(stored_conn *sconn, modb_ref *modb,
                            unsigned int user_id, unsigned int group_id)
{
  int qry_ret;
  char *table;

  if ((table = modbTableName(modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE), 0)) == 0) {
    return 0;
  }

  qry_ret = hasIdMap(sconn, table, "group_id", "user_id", group_id, user_id);
  free(table);

  return qry_ret;
}
int modbLink_Group_User(stored_conn *sconn, modb_ref *modb,
                        unsigned int user_id, unsigned int group_id)
{
  int qry_ret;
  char *table;

  if ((table = modbTableName(modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE), 0)) == 0) {
    return 0;
  }

  qry_ret = addIdMap(sconn, table, "group_id", "user_id", group_id, user_id);
  free(table);

  return qry_ret;
}
int modbUnlink_Group_User(stored_conn *sconn, modb_ref *modb,
                          unsigned int user_id, unsigned int group_id)
{
  int qry_ret;
  char *table;

  if ((table = modbTableName(modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE), 0)) == 0) {
    return 0;
  }

  qry_ret = removeIdMap(sconn, table, "group_id", "user_id", group_id, user_id);
  free(table);

  return qry_ret;
}
