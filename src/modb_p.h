#ifndef H__MODB_P__
#define H__MODB_P__

#include <stddef.h>

#include "modb_types.h"


#define SYS_TABLE "_sys"
#define META_TABLE "_meta"
#define OBJECTS_TABLE "_objects"
#define MDO_GROUPS_TABLE "_mdo_groups"

#define USERS_TABLE "_users"
#define GROUPS_TABLE "_groups"
#define USER_GROUPS_TABLE "_user_groups"

#define META_EXT_TABLE "_meta_ext"


typedef struct str_builder_t str_builder;

char *modbTableName(struct modb_t *modb, const char *suffix, size_t suffix_len);
void modbTableName_sb(str_builder *sb, struct modb_t *modb, const char *suffix, size_t suffix_len);


#endif // H__MODB_P__
