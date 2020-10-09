#ifndef H__MODB_USERS__
#define H__MODB_USERS__

#include "database.h"
#include "modb_types.h"


// User object
struct user_t {
  unsigned int id;

  char *username;
  size_t username_len;

  char *email;
  size_t email_len;

  int64_t created_on;
  int64_t updated_on;
  int64_t deleted_on;

  unsigned int *group_ids;
  struct group_t **groups;
  size_t n_groups;
};

struct user_t *allocUser(void);
struct user_t **allocUsers(size_t n_users);
void freeUser(struct user_t **user);
void freeUsers(struct user_t ***users_ptr, size_t n_users);


// MODB Users
int modbUserById(stored_conn *sconn, modb_ref *modb, unsigned int id, int with_groups,
                 struct user_t **user);
int modbUserByName(stored_conn *sconn, modb_ref *modb, const char *username, int with_groups,
                   struct user_t **user);
int modbUserByEmail(stored_conn *sconn, modb_ref *modb, const char *email, int with_groups,
                   struct user_t **user);
int modbUserSearch(stored_conn *sconn, modb_ref *modb, const char *username_email, int with_groups,
                   struct user_t **user);

int modbUserList(stored_conn *sconn, modb_ref *modb, int with_deleted, int with_groups,
                 struct user_t ***users, size_t *n_users);

int64_t modbUserCreate(stored_conn *sconn, modb_ref *modb,
                       unsigned int id, const char *username, const char *email);
int64_t modbUserUpdate(stored_conn *sconn, modb_ref *modb, unsigned int id,
                       const char *username, const char *email);
int modbUserDelete(stored_conn *sconn, modb_ref *modb, unsigned int id);
int modbUserDestroy(stored_conn *sconn, modb_ref *modb, unsigned int id);


// MODB Users -> Groups
int modbFetchUserGroupIds(stored_conn *sconn, modb_ref *modb,
                          struct user_t *user, int with_deleted);
int modbFetchUserGroups(stored_conn *sconn, modb_ref *modb, struct user_t *user, int with_deleted);

int modbSyncUserGroups(stored_conn *sconn, modb_ref *modb,
                       unsigned int user_id, size_t n_groups, unsigned int *group_id);
int modbSyncUserGroups_va(stored_conn *sconn, modb_ref *modb,
                          unsigned int user_id, size_t n_groups, ...);

int modbIsLinked_User_Group(stored_conn *sconn, modb_ref *modb,
                            unsigned int user_id, unsigned int group_id);
int modbLink_User_Group(stored_conn *sconn, modb_ref *modb,
                        unsigned int user_id, unsigned int group_id);
int modbUnlink_User_Group(stored_conn *sconn, modb_ref *modb,
                          unsigned int user_id, unsigned int group_id);

#endif // H__MODB_USERS__
