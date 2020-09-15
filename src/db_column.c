#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <mysql.h>

#include "db_column.h"


size_t columnTypeToByteSize(enum e_column_type type)
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
      return sizeof(uint32_t);

    case TYPE_RAW:
      return sizeof(char *);
  }
  return sizeof(char *);
}

enum e_column_type simplifyFieldType(enum enum_field_types sql_type, int is_unsigned)
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


struct column_data_t *columnFromResult(struct stored_conn_t *sconn, MYSQL_RES *result,
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

  col = (struct column_data_t *)malloc(sizeof(struct column_data_t));
  if (col == NULL) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }
  memset(col, 0, sizeof(struct column_data_t));

  col->name_size = field->name_length;
  col->name = (char*)malloc(field->name_length + 1);
  if (col->name == NULL) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    free(col);
    return 0;
  }
  memcpy(col->name, field->name, col->name_size);
  *(col->name + col->name_size) = '\0';

  col->type = simplifyFieldType(field->type, (field->flags & UNSIGNED_FLAG) > 0);
  col->type_bytes = columnTypeToByteSize(col->type);

  col->n_values = num_rows;

  col->hasPointers = col->type == TYPE_STRING
    || col->type == TYPE_BLOB
    || col->type == TYPE_RAW;
  col->isNullable  = (field->flags & NOT_NULL_FLAG) == 0;
  col->isBlob      = (field->flags & BLOB_FLAG) > 0;
  col->isTimestamp = (field->flags & TIMESTAMP_FLAG) > 0;

  if (num_rows == 0) {
    return col;
  }

  col->data.vptr = malloc(col->type_bytes * num_rows);
  if (col->data.vptr == NULL) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    free(col->name);
    free(col);
    return 0;
  }
  memset(col->data.vptr, 0, col->type_bytes * num_rows);

  if (col->hasPointers) {
    col->data_sizes = (size_t *)malloc(sizeof(size_t) * num_rows);
    if (col->data_sizes == NULL) {
      fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
      free(col->data.vptr);
      free(col->name);
      free(col);
      return 0;
    }
    memset(col->data_sizes, 0, sizeof(size_t) * num_rows);
  }

  if (col->isNullable) {
    col->nulls = malloc(num_rows / 8 + 1);
    if (col->nulls == NULL) {
      fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
      if (col->data_sizes) {
        free(col->data_sizes);
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
        free(*(col->data.ptr_str + r));
        *(col->data.ptr_str + r) = 0;
      }
    }

    free(col->data_sizes);
    col->data_sizes = 0;
  }

  if (col->data.vptr) {
    free(col->data.vptr);
    col->data.vptr = 0;
  }

  if (col->isNullable) {
    free(col->nulls);
    col->nulls = 0;
  }

  free(col->name);
  col->name = 0;

  free(col);
}
void freeColumns(struct column_data_t **col_data, size_t n_cols)
{
  for (size_t c = 0; c < n_cols; c++) {
    freeColumn(*(col_data + c));
  }
  free(col_data);
}


int setColumnValue(struct column_data_t *col, uint64_t row, const char *value, size_t value_size)
{
  if (value == NULL) {
    columnRowSetNull(col, row);
    return 0;
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
      *(col->data.ptr_uint32 + row) = (uint32_t)strtoul(value, NULL, 10);
      break;
    }
    case TYPE_STRING:
    case TYPE_BLOB:
    case TYPE_RAW:
    {
      *(col->data.ptr_str + row) = (char *)malloc(value_size + 1);
      if (*(col->data.ptr_str + row) == 0) {
        fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
        return -1;
      }
      memcpy(*(col->data.ptr_str + row), value, value_size);
      *(*(col->data.ptr_str + row) + value_size) = 0;
      *(col->data_sizes + row) = value_size;
      break;
    }
  }

  return 0;
}

