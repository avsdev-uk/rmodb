#ifndef H__MODB_P__
#define H__MODB_P__

#include <stddef.h>

#include "modb_types.h"
#include "database.h"
#include "strext.h"


#define SYS_TABLE "_sys"
#define METADATA_TABLE "_metadata"
#define OBJECTS_TABLE "_objects"
#define MDO_GROUPS_TABLE "_mdo_groups"

#define USERS_TABLE "_users"
#define GROUPS_TABLE "_groups"
#define USER_GROUPS_TABLE "_user_groups"

#define META_EXT_TABLE "_meta_ext"


char *modbTableName(char **name, size_t *len, modb_ref *modb, const char *suffix, size_t suffix_len);
void modbFreeTableName(char **name);
void modbTableName_sb(str_builder *sb, modb_ref *modb, const char *suffix, size_t suffix_len);

int moveColumnStrPointer(column_data *col, size_t row, int move, char **target, size_t *target_len);
int moveColumnBlobPointer(column_data *col, size_t row, int move,
                          char **target, size_t *target_len);


#endif // H__MODB_P__
