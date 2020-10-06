#include "modb_p.h"
#include "strext.h"

char *modbTableName(modb_ref *modb, const char *suffix, size_t suffix_len)
{
  str_builder *sb;
  char *str;
  size_t len;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }

  modbTableName_sb(sb, modb, suffix, suffix_len);

  if (strbld_finalize_or_destroy(&sb, &str, &len) != 0) {
    return 0;
  }

  return str;
}

void modbTableName_sb(str_builder *sb, modb_ref *modb, const char *suffix, size_t suffix_len)
{
  strbld_str(sb, modb->name, modb->name_len);
  strbld_str(sb, suffix, suffix_len);
}


char *modbColumnName(modb_ref *modb,
                     const char *table, size_t table_len,
                     const char *column, size_t column_len)
{
  str_builder *sb;
  char *str;
  size_t len;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }

  modbColumnName_sb(sb, modb, table, table_len, column, column_len);

  if (strbld_finalize_or_destroy(&sb, &str, &len) != 0) {
    return 0;
  }

  return str;
}
void modbColumnName_sb(str_builder *sb, modb_ref *modb,
                       const char *table, size_t table_len,
                       const char *column, size_t column_len)
{
  if (table != 0) {
    strbld_char(sb, '`');
    modbTableName_sb(sb, modb, table, table_len);
    strbld_char(sb, '`');
    strbld_char(sb, '.');
  }
  strbld_char(sb, '`');
  strbld_str(sb, column, column_len);
  strbld_char(sb, '`');
}

char *modbColumnNameAs(modb_ref *modb,
                       const char *table, size_t table_len,
                       const char *column, size_t column_len,
                       const char *as_column, size_t as_column_len)
{
  str_builder *sb;
  char *str;
  size_t len;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }

  modbColumnNameAs_sb(sb, modb, table, table_len, column, column_len, as_column, as_column_len);

  if (strbld_finalize_or_destroy(&sb, &str, &len) != 0) {
    return 0;
  }

  return str;
}
void modbColumnNameAs_sb(str_builder *sb, modb_ref *modb,
                         const char *table, size_t table_len,
                         const char *column, size_t column_len,
                         const char *as_column, size_t as_column_len)
{
  if (table != 0) {
    strbld_char(sb, '`');
    modbTableName_sb(sb, modb, table, table_len);
    strbld_char(sb, '`');
    strbld_char(sb, '.');
  }
  strbld_char(sb, '`');
  strbld_str(sb, column, column_len);
  strbld_char(sb, '`');
  strbld_str(sb, " AS ", 4);
  strbld_char(sb, '`');
  strbld_str(sb, as_column, as_column_len);
  strbld_char(sb, '`');
}
