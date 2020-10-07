#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "modb_types.h"
#include "strext.h"


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
    freeGroups(user->groups, user->n_groups);
    user->groups = 0;
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
void freeUsers(struct user_t **users, size_t n_users)
{
  size_t idx;

  for (idx = 0; idx < n_users; idx++) {
    if (*(users + idx) != 0) {
      freeUser((users + idx));
    }
  }

  free(users);
}


struct group_t *allocGroup(void)
{
  struct group_t *group;

  group = malloc(sizeof(struct group_t));
  if (group == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
  }
  memset(group, 0, sizeof(struct group_t));

  return group;
}
struct group_t **allocGroups(size_t n_groups)
{
  struct group_t **groups;

  groups = (struct group_t **)malloc(sizeof(struct group_t *) * n_groups);
  if (groups == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }
  memset(groups, 0, sizeof(struct group_t *) * n_groups);

  return groups;
}
void freeGroup(struct group_t **group_ptr)
{
  struct group_t *group = *group_ptr;

  if (group->n_members > 0) {
    freeUsers(group->members, group->n_members);
    group->members = 0;
    group->n_members = 0;
  }

  if (group->name != 0) {
    free(group->name);
    group->name = 0;
  }

  free(group);
  *group_ptr = 0;
}
void freeGroups(struct group_t **groups, size_t n_groups)
{
  size_t idx;

  for (idx = 0; idx < n_groups; idx++) {
    if (*(groups + idx) != 0) {
      freeGroup((groups + idx));
    }
  }

  free(groups);
}
