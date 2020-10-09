#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "modb_users.h"
#include "modb_groups.h"
#include "modb_p.h"


// ##### PRIVATE
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

  col_id = findColumnByName(col_data, n_cols, "id");
  col_username = findColumnByName(col_data, n_cols, "username");
  col_email = findColumnByName(col_data, n_cols, "email");
  col_created = findColumnByName(col_data, n_cols, "created");
  col_updated = findColumnByName(col_data, n_cols, "updated");
  col_deleted = findColumnByName(col_data, n_cols, "deleted");

  *users = (struct user_t **)malloc(sizeof(struct user_t *) * n_rows);
  if (*users == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return -1;
  }
  memset(*users, 0, sizeof(struct user_t *) * n_rows);

  for (idx = 0; idx < n_rows; idx++) {
    user = allocUser();
    if (user == 0) {
      freeUsers(users, idx - 1);
      return -1;
    }

    user->id = *(col_id->data.ptr_uint32 + idx);

    if (!moveColumnStrPointer(col_username, idx, 1, &user->username, &user->username_len)) {
      freeUsers(users, idx);
      return -1;
    }

    if (!moveColumnStrPointer(col_email, idx, 1, &user->email, &user->email_len)) {
      freeUsers(users, idx);
      return -1;
    }

    user->created_on = *(col_created->data.ptr_int64 + idx);
    if (!columnRowIsNull(col_updated, idx)) {
      user->updated_on = *(col_updated->data.ptr_int64 + idx);
    }
    if (!columnRowIsNull(col_deleted, idx)) {
      user->deleted_on = *(col_deleted->data.ptr_int64 + idx);
    }

    *(*users + idx) = user;
  }

  *n_users = n_rows;

  return 1;
}
int doUsersQuery(stored_conn *sconn, modb_ref *modb, where_builder *wb,
                 struct user_t ***users, size_t *n_users)
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
  modbTableName(&table, &table_len, modb, USERS_TABLE, strlen(USERS_TABLE));

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

  res = tableRowsToUsers(col_data, n_cols, users, n_users);
  freeColumns(col_data, n_cols);

  if (res <= 0) {
    return res;
  }

  return (int)qry_ret;
}
int doScalarUsersQuery(stored_conn *sconn, modb_ref *modb,
                       where_builder *wb, int with_groups, struct user_t **user)
{
  int res;
  struct user_t **users;
  size_t n_users;

  res = doUsersQuery(sconn, modb, wb, &users, &n_users);
  if (res <= 0) {
    return res;
  }

  *user = *(users + 0);
  *(users + 0) = 0;
  freeUsers(&users, n_users);

  if (with_groups) {
    if (modbFetchUserGroups(sconn, modb, *user, 0) < 0) {
      freeUser(user);
      return -1;
    }
  }

  return res;
}


// ##### PUBLIC
struct user_t *allocUser(void)
{
  struct user_t *user;

  user = malloc(sizeof(struct user_t));
  if (user == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
  }
  memset(user, 0, sizeof(struct user_t));

  return user;
}
struct user_t **allocUsers(size_t n_users)
{
  struct user_t **users;

  users = (struct user_t **)malloc(sizeof(struct user_t *) * n_users);
  if (users == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }
  memset(users, 0, sizeof(struct user_t *) * n_users);

  return users;
}
void freeUser(struct user_t **user_ptr)
{
  struct user_t *user = *user_ptr;

  if (user->n_groups > 0) {
    if (user->group_ids != 0) {
      free(user->group_ids);
      user->group_ids = 0;
    }
    if (user->groups != 0) {
      freeGroups(&user->groups, user->n_groups);
    }
    user->n_groups = 0;
  }

  if (user->email != 0) {
    free(user->email);
    user->email = 0;
  }

  if (user->username != 0) {
    free(user->username);
    user->username = 0;
  }

  free(user);
  *user_ptr = 0;
}
void freeUsers(struct user_t ***users_ptr, size_t n_users)
{
  size_t idx;
  struct user_t ** users = *users_ptr;

  for (idx = 0; idx < n_users; idx++) {
    if (*(users + idx) != 0) {
      freeUser((users + idx));
    }
  }

  free(users);
  *users_ptr = 0;
}


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
  qry_ret = addIdMap(sconn, table, table_len, "user_id", "group_id", user_id, group_id);
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
  qry_ret = removeIdMap(sconn, table, table_len, "user_id", "group_id", user_id, group_id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}
