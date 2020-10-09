#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "modb_p.h"
#include "strext.h"

char *modbTableName(char **name, size_t *len, modb_ref *modb, const char *suffix, size_t suffix_len)
{
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  modbTableName_sb(sb, modb, suffix, suffix_len);
  if (strbld_finalize_or_destroy(&sb, name, len) != 0) {
    return 0;
  }

  return *name;
}
void modbTableName_sb(str_builder *sb, modb_ref *modb, const char *suffix, size_t suffix_len)
{
  strbld_str(sb, modb->name, modb->name_len);
  strbld_str(sb, suffix, suffix_len);
}
void modbFreeTableName(char **name)
{
  if (*name != 0) {
    free(*name);
    *name = 0;
  }
}


int moveColumnStrPointer(column_data *col, size_t row, int move, char **target, size_t *target_len)
{
  if (columnRowIsNull(col, row)) {
    return -1;
  }

  if (move) {
    *target = *(col->data.ptr_str + row);
    *(col->data.ptr_str + row) = 0;
    *target_len = *(col->data_lens + row);
    *(col->data_lens + row) = 0;
  } else {
    if (strmemcpy(*(col->data.ptr_str + row), *(col->data_lens + row), target, target_len) != 0) {
      return 0;
    }
  }

  return 1;
}
int moveColumnBlobPointer(column_data *col, size_t row, int move,
                          char **target, size_t *target_len)
{
  if (columnRowIsNull(col, row)) {
    return -1;
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
      return 0;
    }

    memcpy(*target, *(col->data.ptr_blob + row), *(col->data_lens + row));
    *target_len = *(col->data_lens + row);
  }

  return 1;
}

