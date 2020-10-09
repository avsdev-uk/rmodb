#ifndef H__MODB_GROUPS__
#define H__MODB_GROUPS__

#include "database.h"
#include "modb_types.h"

// Group object
struct group_t {
  unsigned int id;

  char *name;
  size_t name_len;

  int64_t created_on;
  int64_t updated_on;
  int64_t deleted_on;

  unsigned int *member_ids;
  struct user_t **members;
  size_t n_members;
};

struct group_t *allocGroup(void);
struct group_t **allocGroups(size_t n_groups);
void freeGroup(struct group_t **group);
void freeGroups(struct group_t ***groups_ptr, size_t n_groups);


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

// MODB Groups -> Users
int modbSyncGroupUsers(stored_conn *sconn, modb_ref *modb,
                       unsigned int group_id, size_t n_users, unsigned int *user_id);
int modbSyncGroupUsers_va(stored_conn *sconn, modb_ref *modb,
                          unsigned int group_id, size_t n_users, ...);

int modbIsLinked_Group_User(stored_conn *sconn, modb_ref *modb,
                            unsigned int group_id, unsigned int user_id);
int modbLink_Group_User(stored_conn *sconn, modb_ref *modb,
                        unsigned int group_id, unsigned int user_id);
int modbUnlink_Group_User(stored_conn *sconn, modb_ref *modb,
                          unsigned int group_id, unsigned int user_id);

#endif // H__MODB_GROUPS__
