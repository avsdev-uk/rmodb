#ifndef H__MODB_ACCOUNTING_P__
#define H__MODB_ACCOUNTING_P__

#include "database.h"
#include "modb_types.h"

// ##### USERS
int tableRowsToUsers(column_data **col_data, size_t n_cols,
                     struct user_t ***users, size_t *n_users);
int doUsersQuery(stored_conn *sconn, modb_ref *modb, where_builder *wb,
                 struct user_t ***users, size_t *n_users);
int doScalarUsersQuery(stored_conn *sconn, modb_ref *modb,
                       where_builder *wb, struct user_t **user);

// ##### GROUPS
int tableRowsToGroups(column_data **col_data, size_t n_cols,
                      struct group_t ***groups, size_t *n_groups);
int doGroupsQuery(stored_conn *sconn, modb_ref *modb, where_builder *wb,
                  struct group_t ***groups, size_t *n_groups);
int doScalarGroupsQuery(stored_conn *sconn, modb_ref *modb,
                        where_builder *wb, struct group_t **group);

#endif // H__MODB_ACCOUNTING_P__
