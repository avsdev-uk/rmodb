#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "modb_p.h"
#include "strext.h"

char *modbTableName(modb_ref *modb, const char *suffix, size_t suffix_len, char encap)
{
  str_builder *sb;
  char *str;
  size_t len;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }

  modbTableName_sb(sb, modb, suffix, suffix_len, encap);

  if (strbld_finalize_or_destroy(&sb, &str, &len) != 0) {
    return 0;
  }

  return str;
}

void modbTableName_sb(str_builder *sb, modb_ref *modb, const char *suffix, size_t suffix_len,
                      char encap)
{
  if (encap != 0) {
    strbld_char(sb, encap);
  }
  strbld_str(sb, modb->name, modb->name_len);
  strbld_str(sb, suffix, suffix_len);
  if (encap != 0) {
    strbld_char(sb, encap);
  }
}


char *modbJoin(modb_ref *modb,
              const char *join, size_t join_len, int equals,
              const char *tableA, size_t tableA_len, const char *colA, size_t colA_len,
              const char *tableB, size_t tableB_len, const char *colB, size_t colB_len)
{
  str_builder *sb;
  char *str;
  size_t len;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }

  modbJoin_sb(sb, modb,
              join, join_len, equals,
              tableA, tableA_len, colA, colA_len,
              tableB, tableB_len, colB, colB_len);

  if (strbld_finalize_or_destroy(&sb, &str, &len) != 0) {
    return 0;
  }

  return str;
}
void modbJoin_sb(str_builder *sb, modb_ref *modb,
                 const char *join, size_t join_len, int equals,
                 const char *tableA, size_t tableA_len, const char *colA, size_t colA_len,
                 const char *tableB, size_t tableB_len, const char *colB, size_t colB_len)
{
  strbld_str(sb, join, join_len);
  strbld_str(sb, " JOIN ", 6);
  modbTableName_sb(sb, modb, tableA, tableA_len, '`');
  strbld_str(sb, " ON ", 4);
  modbColumnName_sb(sb, modb, tableA, tableA_len, colA, colA_len);
  strbld_str(sb, (equals ? " = " : " != "), (equals ? 3 : 4));
  modbColumnName_sb(sb, modb, tableB, tableB_len, colB, colB_len);
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
    modbTableName_sb(sb, modb, table, table_len, '`');
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
    modbTableName_sb(sb, modb, table, table_len, '`');
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


int moveColumnStrPointer(column_data *col, size_t row, int move, char **target, size_t *target_len)
{
  if (columnRowIsNull(col, row)) {
    return 0;
  }

  if (move) {
    *target = *(col->data.ptr_str + row);
    *(col->data.ptr_str + row) = 0;
    *target_len = *(col->data_lens + row);
    *(col->data_lens + row) = 0;
  } else {
    if (strmemcpy(*(col->data.ptr_str + row), *(col->data_lens + row), target, target_len) != 0) {
      return -1;
    }
  }

  return 0;
}
int moveColumnBlobPointer(column_data *col, size_t row, int move,
                          char **target, size_t *target_len)
{
  if (columnRowIsNull(col, row)) {
    return 0;
  }

  if (move) {
    *target = *(col->data.ptr_blob + row);
    *(col->data.ptr_blob + row) = 0;
    *target_len = *(col->data_lens + row);
    *(col->data_lens + row) = 0;
  } else {
    *target = (char *)malloc(*(col->data_lens + row));
    if (*target == 0) {
      fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
      return -1;
    }

    memcpy(*target, *(col->data.ptr_blob + row), *(col->data_lens + row));
    *target_len = *(col->data_lens + row);
  }

  return 0;
}
