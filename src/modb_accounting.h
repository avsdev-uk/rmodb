#ifndef H__MODB_ACCOUNTING__
#define H__MODB_ACCOUNTING__

#include "database.h"
#include "modb_types.h"


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

int modbFetchUserGroupIds(stored_conn *sconn, modb_ref *modb,
                          struct user_t *user, int with_deleted);
int modbFetchUserGroups(stored_conn *sconn, modb_ref *modb, struct user_t *user, int with_deleted);


// MODB Groups
int modbGroupById(stored_conn *sconn, modb_ref *modb, unsigned int id, int with_members,
                  struct group_t **group);
int modbGroupByName(stored_conn *sconn, modb_ref *modb, const char *name, int with_members,
                    struct group_t **group);

int modbGroupList(stored_conn *sconn, modb_ref *modb, int with_deleted, int with_members,
                  struct group_t ***groups, size_t *n_groups);

int64_t modbGroupCreate(stored_conn *sconn, modb_ref *modb,
                        unsigned int id, const char *name);
int64_t modbGroupUpdate(stored_conn *sconn, modb_ref *modb, unsigned int id,
                        const char *name);
int modbGroupDelete(stored_conn *sconn, modb_ref *modb, unsigned int id);
int modbGroupDestroy(stored_conn *sconn, modb_ref *modb, unsigned int id);

int modbFetchGroupUserIds(stored_conn *sconn, modb_ref *modb, struct group_t *group,
                          int with_deleted);
int modbFetchGroupUsers(stored_conn *sconn, modb_ref *modb, struct group_t *group,
                        int with_deleted);


// MODB Users <-> Groups
int modbSyncUserGroups(stored_conn *sconn, modb_ref *modb,
                       unsigned int user_id, size_t n_groups, unsigned int *group_id);
int modbSyncUserGroups_va(stored_conn *sconn, modb_ref *modb,
                          unsigned int user_id, size_t n_groups, ...);
int modbSyncGroupUsers(stored_conn *sconn, modb_ref *modb,
                       unsigned int group_id, size_t n_users, unsigned int *user_id);
int modbSyncGroupUsers_va(stored_conn *sconn, modb_ref *modb,
                          unsigned int group_id, size_t n_users, ...);


int modbIsLinked_Group_User(stored_conn *sconn, modb_ref *modb,
                            unsigned int user_id, unsigned int group_id);
int modbLink_Group_User(stored_conn *sconn, modb_ref *modb,
                        unsigned int user_id, unsigned int group_id);
int modbUnlink_Group_User(stored_conn *sconn, modb_ref *modb,
                          unsigned int user_id, unsigned int group_id);


#endif // H__MODB_ACCOUNTING__
