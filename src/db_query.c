#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <mysql.h>
#include "db_connection.h"
#include "db_column.h"


uint64_t simpleQuery(struct stored_conn_t *sconn, const char *qry, size_t qry_len)
{
#ifdef DEBUG
  printf("QRY: %s\n", qry);
#endif

  if (mysql_real_query(SQCONN(sconn), qry, qry_len) != 0) {
    fprintf(
      stderr, "[%d]mysql_real_query: (%d) %s\n",
      __LINE__, mysql_errno(SQCONN(sconn)), mysql_error(SQCONN(sconn))
    );
    return (uint64_t)-1;
  }

  return mysql_insert_id(SQCONN(sconn));
}

uint64_t tableQuery(struct stored_conn_t *sconn, const char *qry, size_t qry_len, int scalar_result,
                    struct column_data_t ***col_data_ptr, size_t *n_cols)
{
  my_ulonglong insertId;
  my_ulonglong n_rows;
  MYSQL_RES *result;
  MYSQL_ROW row;
  unsigned long *lens;
  int failed = 0;
  struct column_data_t **col_data;

  insertId = simpleQuery(sconn, qry, qry_len);
  if (insertId == (uint64_t)-1) {
    return insertId;
  }

  *n_cols = mysql_field_count(SQCONN(sconn));
  if (scalar_result && (*n_cols) > 1) {
    fprintf(
      stderr,
      "[%d]tableQuery: Scalar result expected, got %ul columns. Using first column value\n",
      __LINE__, (unsigned int)*n_cols
    );
    (*n_cols) = 1;
  }
  if (*n_cols == 0) {
    // insert/update query
    // TODO: res = makeInt64() // lastInsertId
    return insertId;
  }

  result = mysql_store_result(SQCONN(sconn));
  if (result == NULL) {
    fprintf(
      stderr, "[%d]mysql_real_query: (%d) %s\n",
      __LINE__, mysql_errno(SQCONN(sconn)), mysql_error(SQCONN(sconn))
    );
    return (uint64_t)-1;
  }


  col_data = (struct column_data_t **)malloc(sizeof(struct column_data_t *) * *n_cols);
  if (col_data == NULL) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    mysql_free_result(result);
    return insertId;
  }
  memset(col_data, 0, sizeof(struct column_data_t *) * *n_cols);

  n_rows = mysql_num_rows(result);
  if (scalar_result && n_rows > 1) {
    // TODO: throw warning due to column sizing issue
    n_rows = 1;
  }


  for (unsigned int i = 0; i < *n_cols; i++) {
    *(col_data + i) = columnFromResult(sconn, result, n_rows);

    if (*(col_data + i) == 0) {
      failed++;
    }
  }
  if (failed > 0) {
    for (unsigned int i = 0; i < *n_cols; i++) {
      if (*(col_data + i) != 0) {
        freeColumn(*(col_data + i));
      }
    }
    free(col_data);
    mysql_free_result(result);
    return insertId;
  }

  for (my_ulonglong r = 0; r < n_rows; r++) {
    row = mysql_fetch_row(result);
    lens = mysql_fetch_lengths(result);

    if (row == NULL) {
      fprintf(
        stderr, "[%d]mysql_fetch_row: (%d) %s\n",
        __LINE__, mysql_errno(SQCONN(sconn)), mysql_error(SQCONN(sconn))
      );
      return insertId;
    }

    for (unsigned int c = 0; c < *n_cols; c++) {
      struct column_data_t *col = (*(col_data + c));

      if (setColumnValue(col, r, *(row + c), *(lens + c)) < 0) {
        failed++;
      }
    }

    if (failed > 0) {
      r = n_rows;
    }
  }

  if (failed > 0) {
    for (size_t c = 0; c < *n_cols; c++) {
      if (*(col_data + c) != 0) {
        freeColumn(*(col_data + c));
      }
    }
    free(col_data);
    mysql_free_result(result);
    return insertId;
  }

  mysql_free_result(result);

  *col_data_ptr = col_data;

  return n_rows;
}


int scalarInt(struct stored_conn_t *sconn, const char *qry, size_t qry_len, int default_value)
{
  struct column_data_t **col_data;
  size_t n_cols;
  uint64_t n_rows = tableQuery(sconn, qry, qry_len, 1, &col_data, &n_cols);

  if (n_rows == (uint64_t)-1) {
    return default_value;
  }

  if (n_rows > 0) {
    if (!((*col_data)->isNullable && columnRowIsNull(*col_data, 0))) {
      default_value = *((*col_data)->data.ptr_int32);
    }
  }

  freeColumn(*col_data);
  free(col_data);

  return default_value;
}

unsigned int scalarUInt(struct stored_conn_t *sconn, const char *qry, size_t qry_len,
                        unsigned int default_value)
{
  struct column_data_t **col_data;
  size_t n_cols;
  uint64_t n_rows = tableQuery(sconn, qry, qry_len, 1, &col_data, &n_cols);

  if (n_rows == (uint64_t)-1) {
    return default_value;
  }

  if (n_rows > 0) {
    if (!((*col_data)->isNullable && columnRowIsNull(*col_data, 0))) {
      default_value = *((*col_data)->data.ptr_uint32);
    }
  }

  freeColumn(*col_data);
  free(col_data);

  return default_value;
}

double scalarReal(struct stored_conn_t *sconn, const char *qry, size_t qry_len,
                  double default_value)
{
  struct column_data_t **col_data;
  size_t n_cols;
  uint64_t n_rows = tableQuery(sconn, qry, qry_len, 1, &col_data, &n_cols);

  if (n_rows == (uint64_t)-1) {
    return default_value;
  }

  if (n_rows > 0) {
    if (!((*col_data)->isNullable && columnRowIsNull(*col_data, 0))) {
      default_value = *((*col_data)->data.ptr_double);
    }
  }

  freeColumn(*col_data);
  free(col_data);

  return default_value;
}

char scalarChar(struct stored_conn_t *sconn, const char *qry, size_t qry_len, char default_value)
{
  struct column_data_t **col_data;
  size_t n_cols;
  uint64_t n_rows = tableQuery(sconn, qry, qry_len, 1, &col_data, &n_cols);

  if (n_rows == (uint64_t)-1) {
    return default_value;
  }

  if (n_rows > 0) {
    if (!((*col_data)->isNullable && columnRowIsNull(*col_data, 0))) {
      default_value = *((*col_data)->data.ptr_str)[0];
    }
  }

  freeColumn(*col_data);
  free(col_data);

  return default_value;
}

char *scalarString(struct stored_conn_t *sconn, const char *qry, size_t qry_len,
                   char *default_value)
{
  struct column_data_t **col_data;
  size_t n_cols;
  char *retval = NULL;
  uint64_t n_rows = tableQuery(sconn, qry, qry_len, 1, &col_data, &n_cols);

  if (n_rows == (uint64_t)-1) {
    return default_value;
  }

  if (n_rows > 0) {
    if (!((*col_data)->isNullable && columnRowIsNull(*col_data, 0))) {
      retval = (char *)malloc((*col_data)->data_lens[0] + 1);
      if (retval == 0) {
        fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
        freeColumn(*col_data);
        free(col_data);
        return NULL;
      }
      memcpy(retval, *((*col_data)->data.ptr_str), (*col_data)->data_lens[0]);
      retval[(*col_data)->data_lens[0]] = '\0';
    }
  }

  freeColumn(*col_data);
  free(col_data);

  return retval;
}
