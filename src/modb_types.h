#ifndef H__MODB_TYPES__
#define H__MODB_TYPES__

#include <stddef.h>
#include <stdint.h>

struct modb_ref_t {
  const char *name;
  size_t name_len;
};
typedef struct modb_ref_t modb_ref;

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


struct user_t *allocUser(void);
struct user_t **allocUsers(size_t n_users);
void freeUser(struct user_t **user);
void freeUsers(struct user_t **users, size_t n_users);

struct group_t *allocGroup(void);
struct group_t **allocGroups(size_t n_groups);
void freeGroup(struct group_t **group);
void freeGroups(struct group_t **groups, size_t n_groups);


#endif // H__MODB_TYPES__
