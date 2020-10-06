#ifndef H__DB_COLUMN__
#define H__DB_COLUMN__

#include <stdint.h>
#include <stddef.h>
#include <mysql.h>

#include "db_connection.h"

enum e_column_type_t {
  TYPE_RAW        = 0 <<  0,
  TYPE_BOOL       = 1 <<  0,
  TYPE_INT8       = 1 <<  1,
  TYPE_UINT8      = 1 <<  2,
  TYPE_INT16      = 1 <<  3,
  TYPE_UINT16     = 1 <<  4,
  TYPE_INT32      = 1 <<  5,
  TYPE_UINT32     = 1 <<  6,
  TYPE_INT64      = 1 <<  7,
  TYPE_UINT64     = 1 <<  8,
  TYPE_FLOAT      = 1 <<  9,
  TYPE_DOUBLE     = 1 << 10,
  TYPE_STRING     = 1 << 11,
  TYPE_BLOB       = 1 << 12,
  TYPE_TIMESTAMP  = 1 << 13
};
typedef enum e_column_type_t e_column_type;

struct column_data_t {
  char *table;
  size_t table_len;

  char *name;
  size_t name_len;

  enum e_column_type_t type;
  size_t type_bytes;

  size_t n_values;

  uint8_t hasPointers :1;
  uint8_t isUnsigned  :1;
  uint8_t isNullable  :1;
  uint8_t isBlob      :1;
  uint8_t isTimestamp :1;

  union {
    void *vptr;

    int8_t *ptr_int8;
    uint8_t *ptr_uint8;

    int16_t *ptr_int16;
    uint16_t *ptr_uint16;

    int32_t *ptr_int32;
    uint32_t *ptr_uint32;

    int64_t *ptr_int64;
    uint64_t *ptr_uint64;

    float *ptr_float;
    double *ptr_double;

    char **ptr_str;
  } data;
  size_t *data_lens;

  uint8_t *nulls;
};


// Null column value handling - maybe convert these to macros?
static inline int columnRowIsNull(struct column_data_t *col, uint64_t row)
{
  return col->isNullable && (*(col->nulls + (row / 8)) & (1 << (row % 8))) > 0;
}
static inline void columnRowSetNull(struct column_data_t *col, uint64_t row)
{
  if (col->isNullable) {
    *(col->nulls + (row / 8)) |= (1 << (row % 8));
  }
}
static inline void columnRowClearNull(struct column_data_t *col, uint64_t row)
{
  if (col->isNullable) {
    *(col->nulls + (row / 8)) &= ~(1 << (row % 8));
  }
}

struct column_data_t *initEmptyColumn(e_column_type type, int nullable, const char *name,
                                      size_t name_len, const char *table, size_t table_len);
struct column_data_t *columnFromResult(struct stored_conn_t *sconn, MYSQL_RES *result,
                                       uint64_t num_rows);
void freeColumn(struct column_data_t *col);
void freeColumns(struct column_data_t **col_data, size_t n_cols);

int setColumnValue(struct column_data_t *col, uint64_t row, const char *value, size_t value_size);

struct column_data_t *selectColumn(struct column_data_t **col_data, size_t n_cols,
                                   const char *name);

#endif // H__DB_COLUMN__
