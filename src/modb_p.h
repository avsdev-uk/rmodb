#ifndef H__MODB_P__
#define H__MODB_P__

#include <stddef.h>

#include "modb_types.h"
#include "strext.h"


#define SYS_TABLE "_sys"
#define META_TABLE "_meta"
#define OBJECTS_TABLE "_objects"
#define MDO_GROUPS_TABLE "_mdo_groups"

#define USERS_TABLE "_users"
#define GROUPS_TABLE "_groups"
#define USER_GROUPS_TABLE "_user_groups"

#define META_EXT_TABLE "_meta_ext"


char *modbTableName(modb_ref *modb, const char *suffix, size_t suffix_len);
void modbTableName_sb(str_builder *sb, modb_ref *modb, const char *suffix, size_t suffix_len);

char *modbColumnName(modb_ref *modb,
                     const char *table, size_t table_len,
                     const char *column, size_t column_len);
void modbColumnName_sb(str_builder *sb, modb_ref *modb,
                       const char *table, size_t table_len,
                       const char *column, size_t column_len);

char *modbColumnNameAs(modb_ref *modb,
                       const char *table, size_t table_len,
                       const char *column, size_t column_len,
                       const char *as_column, size_t as_column_len);
void modbColumnNameAs_sb(str_builder *sb, modb_ref *modb,
                         const char *table, size_t table_len,
                         const char *column, size_t column_len,
                         const char *as_column, size_t as_column_len);


#endif // H__MODB_P__
