#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "modb_p.h"
#include "modb_accounting_p.h"


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
  strbld_str(sb, "SELECT * FROM ", 0);
  modbTableName_sb(sb, modb, USERS_TABLE, strlen(USERS_TABLE), '`');
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
  strbld_str(sb, "SELECT * FROM ", 0);
  modbTableName_sb(sb, modb, GROUPS_TABLE, strlen(GROUPS_TABLE), '`');
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

