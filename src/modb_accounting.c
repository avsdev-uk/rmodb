#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "modb_accounting.h"
#include "modb_types.h"
#include "strext.h"
#include "modb_p.h"


// ##### USERS

int tableRowsToUsers(column_data **col_data, size_t n_cols,
                     struct user_t ***users, size_t *n_users)
{
  column_data *col_id, *col_username, *col_email, *col_created, *col_updated, *col_deleted;
  size_t n_rows, idx;
  struct user_t *user;

  n_rows = (*col_data)->n_values;
  if (n_rows == 0) {
    return 0;
  }

  col_id = findColumn(col_data, n_cols, "id");
  col_username = findColumn(col_data, n_cols, "username");
  col_email = findColumn(col_data, n_cols, "email");
  col_created = findColumn(col_data, n_cols, "created");
  col_updated = findColumn(col_data, n_cols, "updated");
  col_deleted = findColumn(col_data, n_cols, "deleted");

  *users = (struct user_t **)malloc(sizeof(struct user_t *) * n_rows);
  if (*users == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return -errno;
  }
  memset(*users, 0, sizeof(struct user_t *) * n_rows);

  for (idx = 0; idx < n_rows; idx++) {
    user = allocUser();
    if (user == 0) {
      freeUsers(*users, idx - 1);
      return -1;
    }

    user->id = *(col_id->data.ptr_uint32 + idx);
    if (strmemcpy(*(col_username->data.ptr_str + idx), *(col_username->data_lens + idx),
                  &user->username, &user->username_len) != 0) {
      freeUsers(*users, idx);
      return -1;
    }
    if (strmemcpy(*(col_email->data.ptr_str + idx), *(col_email->data_lens + idx),
                  &user->email, &user->email_len) != 0) {
      freeUsers(*users, idx);
      return -1;
    }
    user->created_on = *(col_created->data.ptr_uint32 + idx);
    if (!columnRowIsNull(col_updated, idx)) {
      user->updated_on = *(col_updated->data.ptr_uint32 + idx);
    }
    if (!columnRowIsNull(col_deleted, idx)) {
      user->deleted_on = *(col_deleted->data.ptr_uint32 + idx);
    }

    *(*users + idx) = user;
  }

  *n_users = n_rows;

  return 1;
}
int doUsersQuery(stored_conn *sconn, modb_ref *modb, where_builder *wb,
                 struct user_t ***users, size_t *n_users)
{
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;
  int res;

  str_builder *sb;

  column_data **col_data;
  size_t n_cols;


  if ((sb = strbld_create()) == 0) {
    return -errno;
  }
  strbld_str(sb, "SELECT * FROM `", 0);
  modbTableName_sb(sb, modb, USERS_TABLE, strlen(USERS_TABLE));
  strbld_char(sb, '`');
  if (wb != 0) {
    strbld_str(sb, " WHERE ", 0);
    compileWhereBuilder_sb(wb, sb, 0);
  }
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return -errno;
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

  res = tableRowsToUsers(col_data, n_cols, users, n_users);
  freeColumns(col_data, n_cols);

  if (res <= 0) {
    return res;
  }

  return (int)qry_ret;
}
int doScalarUsersQuery(stored_conn *sconn, modb_ref *modb,
                       where_builder *wb, struct user_t **user)
{
  int res;
  struct user_t **users;
  size_t n_users;

  res = doUsersQuery(sconn, modb, wb, &users, &n_users);
  if (res <= 0) {
    return res;
  }

  *user = *users;
  *users = 0;
  freeUsers(users, n_users);

  return res;
}


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
  strbld_str(sb, "INSERT INTO `", 0);
  modbTableName_sb(sb, modb, USERS_TABLE, strlen(USERS_TABLE));
  strbld_str(sb, "` (`id`, `username`, `email`) VALUES(", 0);
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
  strbld_str(sb, "UPDATE `", 0);
  modbTableName_sb(sb, modb, USERS_TABLE, strlen(USERS_TABLE));
  strbld_str(sb, "` SET `id` = ", 0);
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
  strbld_str(sb, "UPDATE `", 0);
  modbTableName_sb(sb, modb, USERS_TABLE, strlen(USERS_TABLE));
  strbld_str(sb, "` SET `deleted` = CURRENT_TIMESTAMP() WHERE `id` = ", 0);
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
  strbld_str(sb, "DELETE FROM `", 0);
  modbTableName_sb(sb, modb, USERS_TABLE, strlen(USERS_TABLE));
  strbld_str(sb, "` WHERE `id` = ", 0);
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

  col_id = findColumn(col_data, n_cols, "id");
  col_name = findColumn(col_data, n_cols, "name");
  col_created = findColumn(col_data, n_cols, "created");
  col_updated = findColumn(col_data, n_cols, "updated");
  col_deleted = findColumn(col_data, n_cols, "deleted");

  *groups = (struct group_t **)malloc(sizeof(struct group_t *) * n_rows);
  if (*groups == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return -errno;
  }
  memset(*groups, 0, sizeof(struct group_t *) * n_rows);

  for (idx = 0; idx < n_rows; idx++) {
    if ((group = allocGroup()) == 0) {
      freeGroups(*groups, idx - 1);
      return -1;
    }

    group->id = *(col_id->data.ptr_uint32 + idx);
    if (strmemcpy(*(col_name->data.ptr_str + idx), *(col_name->data_lens + idx),
                  &group->name, &group->name_len) != 0) {
      freeGroups(*groups, idx);
      return -1;
    }
    group->created_on = *(col_created->data.ptr_uint32 + idx);
    if (!columnRowIsNull(col_updated, idx)) {
      group->updated_on = *(col_updated->data.ptr_uint32 + idx);
    }
    if (!columnRowIsNull(col_deleted, idx)) {
      group->deleted_on = *(col_deleted->data.ptr_uint32 + idx);
    }

    *(*groups + idx) = group;
  }

  *n_groups = n_rows;

  return 1;
}
int doGroupsQuery(stored_conn *sconn, modb_ref *modb, where_builder *wb,
                  struct group_t ***groups, size_t *n_groups)
{
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
  strbld_str(sb, "SELECT * FROM `", 0);
  modbTableName_sb(sb, modb, GROUPS_TABLE, strlen(GROUPS_TABLE));
  strbld_char(sb, '`');
  if (wb != 0) {
    strbld_str(sb, " WHERE ", 0);
    compileWhereBuilder_sb(wb, sb, 0);
  }
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
                        where_builder *wb, struct group_t **group)
{
  int res;
  struct group_t **groups;
  size_t n_groups;

  res = doGroupsQuery(sconn, modb, wb, &groups, &n_groups);
  if (res <= 0) {
    return res;
  }

  *group = *groups;
  *groups = 0;
  freeGroups(groups, n_groups);

  return res;
}


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
  strbld_str(sb, "INSERT INTO `", 0);
  modbTableName_sb(sb, modb, GROUPS_TABLE, strlen(GROUPS_TABLE));
  strbld_str(sb, "` (`id`, `name`) VALUES(", 0);
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
  strbld_str(sb, "UPDATE `", 0);
  modbTableName_sb(sb, modb, GROUPS_TABLE, strlen(GROUPS_TABLE));
  strbld_str(sb, "` SET `id` = ", 0);
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
  strbld_str(sb, "UPDATE `", 0);
  modbTableName_sb(sb, modb, GROUPS_TABLE, strlen(GROUPS_TABLE));
  strbld_str(sb, "` SET `deleted` = CURRENT_TIMESTAMP() WHERE `id` = ", 0);
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
  strbld_str(sb, "DELETE FROM `", 0);
  modbTableName_sb(sb, modb, GROUPS_TABLE, strlen(GROUPS_TABLE));
  strbld_str(sb, "` WHERE `id` = ", 0);
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

  if ((table = modbTableName(modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE))) == 0) {
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

  if ((table = modbTableName(modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE))) == 0) {
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

  if ((table = modbTableName(modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE))) == 0) {
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

  if ((table = modbTableName(modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE))) == 0) {
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
  strbld_str(sb, "SELECT `group_id` FROM `", 0);
  modbTableName_sb(sb, modb, USER_GROUPS_TABLE, strlen(USER_GROUPS_TABLE));
  strbld_str(sb, "` ", 2);
  modbJoin_sb(sb, modb, "LEFT", 4, 1,
              GROUPS_TABLE, strlen(GROUPS_TABLE), "id", 2,
              USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE), "group_id", 8);
  strbld_str(sb, " WHERE ", 0);
  wb = where(0, "user_id", EQ, TYPE_ID, 1, user_id);
  if (!with_deleted) {
    table = modbTableName(modb, GROUPS_TABLE, STR_LEN(GROUPS_TABLE));
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
  strbld_str(sb, "SELECT `user_id` FROM `", 0);
  modbTableName_sb(sb, modb, USER_GROUPS_TABLE, strlen(USER_GROUPS_TABLE));
  strbld_str(sb, "` ", 2);
  modbJoin_sb(sb, modb, "LEFT", 4, 1,
              USERS_TABLE, strlen(USERS_TABLE), "id", 2,
              USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE), "user_id", 8);
  strbld_str(sb, " WHERE ", 0);
  wb = where(0, "group_id", EQ, TYPE_ID, 1, group_id);
  if (!with_deleted) {
    table = modbTableName(modb, USERS_TABLE, STR_LEN(USERS_TABLE));
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

  if ((table = modbTableName(modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE))) == 0) {
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

  if ((table = modbTableName(modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE))) == 0) {
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

  if ((table = modbTableName(modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE))) == 0) {
    return 0;
  }

  qry_ret = removeIdMap(sconn, table, "group_id", "user_id", group_id, user_id);
  free(table);

  return qry_ret;
}
