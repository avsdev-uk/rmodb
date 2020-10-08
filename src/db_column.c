#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <mysql.h>

#include "db_column.h"
#include "db_value.h"
#include "strext.h"


// PRIVATE
size_t columnTypeToByteSize(e_column_type type)
{
  switch(type) {
    case TYPE_BOOL:
      return sizeof(uint8_t);

    case TYPE_INT8:
    case TYPE_UINT8:
      return sizeof(uint8_t);

    case TYPE_INT16:
    case TYPE_UINT16:
      return sizeof(uint16_t);

    case TYPE_INT32:
    case TYPE_UINT32:
      return sizeof(uint32_t);

    case TYPE_INT64:
    case TYPE_UINT64:
      return sizeof(uint64_t);

    case TYPE_FLOAT:
      return sizeof(float);
    case TYPE_DOUBLE:
      return sizeof(double);

    case TYPE_STRING:
      return sizeof(char *);
    case TYPE_BLOB:
      return sizeof(char *);

    case TYPE_TIMESTAMP:
      return sizeof(int64_t);

    case TYPE_ID:
      return sizeof(uint32_t);

    case TYPE_RAW:
      return sizeof(char *);
  }
  return sizeof(char *);
}
e_column_type simplifyFieldType(enum enum_field_types sql_type, int is_unsigned)
{
  switch(sql_type) {
    case MYSQL_TYPE_TINY:
      return is_unsigned ? TYPE_UINT8 : TYPE_INT8;

    case MYSQL_TYPE_SHORT:
      return is_unsigned ? TYPE_UINT16 : TYPE_INT16;

    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
      return is_unsigned ? TYPE_UINT32 : TYPE_INT32;

    case MYSQL_TYPE_LONGLONG:
    case MYSQL_TYPE_BIT:
      return is_unsigned ? TYPE_UINT64 : TYPE_INT64;

    case MYSQL_TYPE_FLOAT:
      return TYPE_FLOAT;

    case MYSQL_TYPE_DOUBLE:
      return TYPE_DOUBLE;

    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_TIMESTAMP2:
      return TYPE_TIMESTAMP;

    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_VARCHAR:
      return TYPE_STRING;

    case MYSQL_TYPE_JSON:
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_BLOB:
      return TYPE_BLOB;

    case MYSQL_TYPE_YEAR:

    case MYSQL_TYPE_DATE:
    case MYSQL_TYPE_NEWDATE:

    case MYSQL_TYPE_TIME:
    case MYSQL_TYPE_TIME2:

    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_DATETIME2:

    case MYSQL_TYPE_SET:
    case MYSQL_TYPE_ENUM:
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_GEOMETRY:
    case MYSQL_TYPE_NULL:
    default:
      return TYPE_RAW;
  }
}


// PUBLIC
struct column_data_t *initEmptyColumn(e_column_type type, int nullable, const char *name,
                                      size_t name_len, const char *table, size_t table_len)
{
  struct column_data_t *col;

  if (name_len == 0) {
    name_len = strlen(name);
  }

  col = (struct column_data_t *)malloc(sizeof(struct column_data_t));
  if (col == NULL) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }
  memset(col, 0, sizeof(struct column_data_t));

  if (strmemcpy(name, name_len, &col->name, &col->name_len) != 0) {
    free(col);
    return 0;
  }

  if (table != 0) {
    if (strmemcpy(table, table_len, &col->table, &col->table_len) != 0) {
      free(col->name);
      free(col);
      return 0;
    }
  }

  col->type = type;
  col->type_bytes = columnTypeToByteSize(col->type);

  col->n_values = 0;

  col->hasPointers = col->type == TYPE_STRING
      || col->type == TYPE_BLOB
      || col->type == TYPE_RAW;
  col->isUnsigned  = col->type == TYPE_UINT8
      || col->type == TYPE_UINT16
      || col->type == TYPE_UINT32
      || col->type == TYPE_UINT64;
  col->isNullable  = nullable > 0;
  col->isBlob      = type == TYPE_BLOB;
  col->isTimestamp = type == TYPE_TIMESTAMP;
  col->isAutoIncrement = type == TYPE_ID;

  return col;
}
struct column_data_t *createColumnFromResult(struct stored_conn_t *sconn, MYSQL_RES *result,
                                             uint64_t num_rows)
{
  MYSQL_FIELD *field;
  struct column_data_t *col;

  field = mysql_fetch_field(result);
  if (field == NULL) {
    fprintf(
          stderr, "[%d]mysql_fetch_field: (%d) %s\n",
          __LINE__, mysql_errno(SQCONN(sconn)), mysql_error(SQCONN(sconn))
          );
    return 0;
  }

  col = initEmptyColumn(
        simplifyFieldType(field->type, (field->flags & UNSIGNED_FLAG) > 0),
        (field->flags & NOT_NULL_FLAG) == 0,
        field->name,
        field->name_length,
        field->table,
        field->table_length
        );
  col->isAutoIncrement = (field->flags & AUTO_INCREMENT_FLAG) != 0;
  col->isTimestamp = (field->flags & TIMESTAMP_FLAG) != 0;

  if (col == 0 || num_rows == 0) {
    return col;
  }

  col->n_values = num_rows;

  col->data.vptr = malloc(col->type_bytes * num_rows);
  if (col->data.vptr == NULL) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    free(col->name);
    free(col);
    return 0;
  }
  memset(col->data.vptr, 0, col->type_bytes * num_rows);

  if (col->hasPointers) {
    col->data_lens = (size_t *)malloc(sizeof(size_t) * num_rows);
    if (col->data_lens == NULL) {
      fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
      free(col->data.vptr);
      free(col->name);
      free(col);
      return 0;
    }
    memset(col->data_lens, 0, sizeof(size_t) * num_rows);
  }

  if (col->isNullable) {
    col->nulls = malloc(num_rows / 8 + 1);
    if (col->nulls == NULL) {
      fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
      if (col->data_lens) {
        free(col->data_lens);
      }
      free(col->data.vptr);
      free(col->name);
      free(col);
      return 0;
    }
    memset(col->nulls, 0, num_rows / 8 + 1);
  }

  return col;
}

void freeColumn(struct column_data_t *col)
{
  if (col->hasPointers) {
    for (unsigned int r = 0; r < col->n_values; r++) {
      if (!columnRowIsNull(col, r)) {
        if (col->type == TYPE_STRING) {
          if (*(col->data.ptr_str + r) != 0) {
            free(*(col->data.ptr_str + r));
            *(col->data.ptr_str + r) = 0;
          }
        } else {
          if (*(col->data.ptr_blob + r) != 0) {
            free(*(col->data.ptr_blob + r));
            *(col->data.ptr_blob + r) = 0;
          }
        }
      }
    }

    free(col->data_lens);
    col->data_lens = 0;
  }

  if (col->data.vptr) {
    free(col->data.vptr);
    col->data.vptr = 0;
  }

  if (col->isNullable) {
    free(col->nulls);
    col->nulls = 0;
  }

  if (col->table) {
    free(col->table);
    col->table = 0;
  }

  if (col->name) {
    free(col->name);
    col->name = 0;
  }

  free(col);
}
void freeColumns(struct column_data_t **col_data, size_t n_cols)
{
  for (size_t c = 0; c < n_cols; c++) {
    if (*(col_data + c) != 0) {
      freeColumn(*(col_data + c));
    }
  }
  free(col_data);
}

int setColumnValueFromResult(struct column_data_t *col, uint64_t row,
                             const char *value, size_t value_size)
{
  if (value == NULL) {
    columnRowSetNull(col, row);
    return 0;
  }

  if (value_size == 0) {
    value_size = strlen(value);
  }

  switch(col->type) {
    case TYPE_BOOL:
    {
      *(col->data.ptr_uint8 + row) = value[0] == '1';
      break;
    }

    case TYPE_INT8:
    {
      *(col->data.ptr_int8 + row) = (int8_t)strtol(value, NULL, 10);
      break;
    }
    case TYPE_UINT8:
    {
      *(col->data.ptr_uint8 + row) = (uint8_t)strtoul(value, NULL, 10);
      break;
    }

    case TYPE_INT16:
    {
      *(col->data.ptr_int16 + row) = (int16_t)strtol(value, NULL, 10);
      break;
    }
    case TYPE_UINT16:
    {
      *(col->data.ptr_uint16 + row) = (uint16_t)strtoul(value, NULL, 10);
      break;
    }

    case TYPE_INT32:
    {
      *(col->data.ptr_int32 + row) = (int32_t)strtol(value, NULL, 10);
      break;
    }
    case TYPE_UINT32:
    {
      *(col->data.ptr_uint32 + row) = (uint32_t)strtoul(value, NULL, 10);
      break;
    }

    case TYPE_INT64:
    {
      *(col->data.ptr_int64 + row) = strtoll(value, NULL, 10);
      break;
    }
    case TYPE_UINT64:
    {
      *(col->data.ptr_uint64 + row) = strtoull(value, NULL, 10);
      break;
    }

    case TYPE_FLOAT:
    {
      *(col->data.ptr_float + row) = strtof(value, NULL);
      break;
    }
    case TYPE_DOUBLE:
    {
      *(col->data.ptr_double + row) = strtod(value, NULL);
      break;
    }

    case TYPE_TIMESTAMP:
    {
      *(col->data.ptr_int64 + row) = db_timestampUnix(value);
      break;
    }

    case TYPE_ID:
    {
      *(col->data.ptr_uint32 + row) = (uint32_t)strtoul(value, NULL, 10);
      break;
    }

    case TYPE_STRING:
    {
      if (strmemcpy(value, value_size, (col->data.ptr_str + row), (col->data_lens + row)) != 0) {
        return -1;
      }
      break;
    }

    case TYPE_BLOB:
    case TYPE_RAW:
    {
      *(col->data.ptr_blob + row) = (char *)malloc(value_size);
      if (*(col->data.ptr_blob + row) == 0) {
        fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
        return -errno;
      }
      memcpy(*(col->data.ptr_blob + row), value, value_size);
      *(col->data_lens + row) = value_size;
      break;
    }
  }

  return 0;
}

struct column_data_t *findColumnByName(struct column_data_t **col_data, size_t n_cols,
                                       const char *name)
{
  struct column_data_t *col = 0;
  size_t idx = 0;
  size_t name_len = strlen(name);

  while (idx < n_cols) {
    col = *(col_data + idx);
    if (col->name_len == name_len && strncmp(col->name, name, name_len) == 0) {
      return *(col_data + idx);
    }
    idx++;
  }

  return 0;
}


int columnRowIsNull(struct column_data_t *col, uint64_t row)
{
  return col->isNullable && (*(col->nulls + (row / 8)) & (1 << (row % 8))) > 0;
}
void columnRowSetNull(struct column_data_t *col, uint64_t row)
{
  if (col->isNullable) {
    *(col->nulls + (row / 8)) |= (1 << (row % 8));
  }
}
void columnRowClearNull(struct column_data_t *col, uint64_t row)
{
  if (col->isNullable) {
    *(col->nulls + (row / 8)) &= ~(1 << (row % 8));
  }
}


char *escapeColumnName(char **str, size_t *len,
                       const char *table, size_t table_len, const char *col, size_t col_len)
{
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  escapeColumnName_sb(sb, table, table_len, col, col_len);
  if (strbld_finalize_or_destroy(&sb, str, len) != 0) {
    return 0;
  }

  return *str;
}
void escapeColumnName_sb(str_builder *sb,
                         const char *table, size_t table_len, const char *col, size_t col_len)
{
  if (table != 0) {
    strbld_char(sb,'`');
    strbld_str(sb, table, table_len);
    strbld_char(sb,'`');
    strbld_char(sb,'.');
  }
  strbld_char(sb,'`');
  strbld_str(sb, col, col_len);
  strbld_char(sb,'`');
}

char *escapeColumnNameAs(char **str, size_t *len, const char *tbl, size_t tbl_len,
                         const char *col, size_t col_len, const char *as, size_t as_len)
{
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  escapeColumnNameAs_sb(sb, tbl, tbl_len, col, col_len, as, as_len);
  if (strbld_finalize_or_destroy(&sb, str, len) != 0) {
    return 0;
  }

  return *str;
}
void escapeColumnNameAs_sb(str_builder *sb, const char *tbl, size_t tbl_len,
                           const char *col, size_t col_len, const char *as, size_t as_len)
{
  escapeColumnName_sb(sb, tbl, tbl_len, col, col_len);
  strbld_str(sb, " AS ", 4);
  escapeColumnName_sb(sb, 0, 0, as, as_len);
}

char *escapeTableName(char **str, size_t *len, const char *table, size_t table_len)
{
  return escapeColumnName(str, len, 0, 0, table, table_len);
}
void escapeTableName_sb(str_builder *sb, const char *table, size_t table_len)
{
  escapeColumnName_sb(sb, 0, 0, table, table_len);
}


char *columnCreateStr(char **str, size_t *len, struct column_data_t *col)
{
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  columnCreateStr_sb(sb, col);
  if (strbld_finalize_or_destroy(&sb, str, len) != 0) {
    return 0;
  }

  return *str;
}
void columnCreateStr_sb(str_builder *sb, struct column_data_t *col)
{
  escapeColumnName_sb(sb, 0, 0, col->name, col->name_len);
  strbld_char(sb, ' ');

  switch(col->type) {
    case TYPE_RAW:
      strbld_str(sb, "MEDIUMBLOB", 10);
      break;
    case TYPE_BOOL:
      strbld_str(sb, "BOOLEAN", 7);
      break;
    case TYPE_INT8:
    case TYPE_UINT8:
      strbld_str(sb, "TINYINT", 7);
      break;
    case TYPE_INT16:
    case TYPE_UINT16:
      strbld_str(sb, "SMALLINT", 8);
      break;
    case TYPE_INT32:
    case TYPE_UINT32:
      strbld_str(sb, "INT", 3);
      break;
    case TYPE_INT64:
    case TYPE_UINT64:
      strbld_str(sb, "BIGINT", 6);
      break;
    case TYPE_FLOAT:
      strbld_str(sb, "FLOAT", 5);
      break;
    case TYPE_DOUBLE:
      strbld_str(sb, "DOUBLE", 6);
      break;
    case TYPE_STRING:
      strbld_str(sb, "VARCHAR(4096)", 13);
      break;
    case TYPE_BLOB:
      strbld_str(sb, "MEDIUMBLOB", 10);
      break;
    case TYPE_TIMESTAMP:
      strbld_str(sb, "TIMESTAMP", 8);
      break;
    case TYPE_ID:
      strbld_str(sb, "INT", 3);
      break;
  }

  if (col->isUnsigned) {
    strbld_str(sb, " UNSIGNED", 0);
  }

  if (col->isNullable) {
    strbld_str(sb, " NULL", 0);
  } else {
    strbld_str(sb, " NOT NULL", 0);
  }

  if (col->isAutoIncrement) {
    strbld_str(sb, " AUTO_INCREMENT", 0);
  }
}

char *columnSetValueStr(char **set, size_t *set_len,
                        const char *column, e_column_type type, uint32_t n_args, ...)
{
  va_list args;

  va_start(args, n_args);
  columnSetValueStr_va(set, set_len, column, type, n_args, args);
  va_end(args);

  return *set;
}
char *columnSetValueStr_va(char **set, size_t *set_len,
                           const char *column, e_column_type type, uint32_t n_args, va_list args)
{
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  columnSetValueStr_sbva(sb, column, type, n_args, args);
  if (strbld_finalize_or_destroy(&sb, set, set_len) != 0) {
    return 0;
  }

  return *set;
}
void columnSetValueStr_sb(str_builder *sb,
                          const char *column, e_column_type type, uint32_t n_args, ...)
{
  va_list args;
  va_start(args, n_args);
  columnSetValueStr_sbva(sb, column, type, n_args, args);
  va_end(args);
}
void columnSetValueStr_sbva(str_builder *sb,
                            const char *column, e_column_type type, uint32_t n_args, va_list args)
{
  escapeColumnName_sb(sb, 0, 0, column, 0);
  strbld_str(sb, " = ", 0);
  db_value_sbva(sb, type, n_args, args);
}

char *joinStr(char **str, size_t *len,
              const char *join_type, size_t join_type_len, int is_equals,
              const char *tableA, size_t tableA_len, const char *colA, size_t colA_len,
              const char *tableB, size_t tableB_len, const char *colB, size_t colB_len)
{
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  joinStr_sb(sb, join_type, join_type_len, is_equals,
             tableA, tableA_len, colA, colA_len,
             tableB, tableB_len, colB, colB_len);
  if (strbld_finalize_or_destroy(&sb, str, len) != 0) {
    return 0;
  }

  return *str;
}
void joinStr_sb(str_builder *sb,
                const char *join_type, size_t join_type_len, int is_equals,
                const char *tableA, size_t tableA_len, const char *colA, size_t colA_len,
                const char *tableB, size_t tableB_len, const char *colB, size_t colB_len)
{
  strbld_str(sb, join_type, join_type_len);
  strbld_str(sb, " JOIN ", 6);
  escapeTableName_sb(sb, tableA, tableA_len);
  strbld_str(sb, " ON ", 4);
  escapeColumnName_sb(sb, tableA, tableA_len, colA, colA_len);
  strbld_str(sb, (is_equals ? " = " : " != "), (is_equals ? 3 : 4));
  escapeColumnName_sb(sb, tableB, tableB_len, colB, colB_len);
}

