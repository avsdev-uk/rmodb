#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <mysql.h>

#include "db_connection.h"
#include "db_column.h"
#include "db_value.h"
#include "db_where-builder.h"
#include "strext.h"


uint64_t simpleQuery(struct stored_conn_t *sconn, const char *qry, size_t qry_len)
{
#if defined DEBUG || defined SQL_DEBUG
  printf("QRY: %s\n", qry);
#endif

  char *old_ptr;

  if (mysql_real_query(SQCONN(sconn), qry, qry_len) != 0) {
    fprintf(
      stderr, "[%d]mysql_real_query: (%d) %s\n",
      __LINE__, mysql_errno(SQCONN(sconn)), mysql_error(SQCONN(sconn))
    );
    return (uint64_t)-1;
  }

  if (sconn->last_qry_alloc < qry_len + 1) {
    old_ptr = sconn->last_qry;
    sconn->last_qry = realloc(sconn->last_qry, qry_len + 1);
    if (sconn->last_qry == 0) {
      sconn->last_qry = old_ptr;
      fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
      return (uint64_t)-1;
    }
    sconn->last_qry_alloc = qry_len + 1;
  }

  memcpy(sconn->last_qry, qry, qry_len);
  sconn->last_qry[qry_len] = '\0';
  sconn->last_qry_len = qry_len;
  sconn->num_queries++;

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
    fprintf(stderr, "[%d]tableQuery: WARN: scalar_result expects 1 row, "
                    "%llu rows returned from query. Using first result only\n", __LINE__, n_rows);
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
  size_t n_cols, len;
  char *retval = NULL;
  uint64_t n_rows = tableQuery(sconn, qry, qry_len, 1, &col_data, &n_cols);

  if (n_rows == (uint64_t)-1) {
    return default_value;
  }

  if (n_rows > 0) {
    if (!((*col_data)->isNullable && columnRowIsNull(*col_data, 0))) {
      if (strmemcpy(*((*col_data)->data.ptr_str), (*col_data)->data_lens[0], &retval, &len) != 0) {
        freeColumn(*col_data);
        free(col_data);
        return NULL;
      }
    }
  }

  freeColumn(*col_data);
  free(col_data);

  return retval;
}


int64_t countQuery(struct stored_conn_t *sconn, const char *table, where_builder *wb)
{
  char *qry;
  size_t qry_len;
  int qry_ret;
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return -1;
  }
  strbld_str(sb, "SELECT COUNT(*) AS `C` FROM `", 0);
  strbld_str(sb, table, 0);
  strbld_str(sb, "` WHERE ", 0);
  compileWhereBuilder_sb(wb, sb);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    free(wb);
    return -1;
  }
  free(wb);

  qry_ret = scalarInt(sconn, qry, qry_len, 0);
  free(qry);

  return qry_ret;
}

int deleteQuery(struct stored_conn_t *sconn, const char *table, where_builder *wb)
{
  str_builder *sb;
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return -1;
  }
  strbld_str(sb, "DELETE FROM `", 0);
  strbld_str(sb, table, 0);
  strbld_str(sb, "` WHERE ", 0);
  compileWhereBuilder_sb(wb, sb);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return -1;
  }

  qry_ret = simpleQuery(sconn, qry, qry_len);
  free(qry);

  if (qry_ret == (uint64_t)-1) {
    return -1;
  }

  return qry_ret == 0;
}



int syncIdMap(struct stored_conn_t *sconn, const char *table,
              const char *primary_col, const char *map_col,
              unsigned int primary_id, size_t n_maps, unsigned int *map_ids)
{
  str_builder *sb;
  where_builder *wb;
  char *qry;
  size_t qry_len;
  int qry_ret;
  size_t idx;

  wb = where(table, primary_col, EQ, TYPE_ID, 1, primary_id);
  qry_ret = deleteQuery(sconn, table, wb);
  free(wb);

  if (qry_ret != 0) {
    return qry_ret;
  }

  if ((sb = strbld_create()) == 0) {
    return -1;
  }
  strbld_str(sb, "INSERT INTO `", 0);
  strbld_str(sb, table, 0);
  strbld_str(sb, "` (`", 0);
  strbld_str(sb, primary_col, 0);
  strbld_str(sb, "`, `", 0);
  strbld_str(sb, map_col, 0);
  strbld_str(sb, "`) VALUES ", 0);
  for (idx = 0; idx < n_maps; idx++) {
    strbld_char(sb, '(');
    db_value_sb(sb, TYPE_ID, 1, primary_id);
    strbld_char(sb, ',');
    db_value_sb(sb, TYPE_ID, 1, *(map_ids + idx));
    strbld_char(sb, ')');
    if ((idx + 1) < n_maps) {
      strbld_char(sb, ',');
    }
  }
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return -1;
  }

  if (simpleQuery(sconn, qry, qry_len) == (uint64_t)-1) {
    return -1;
  }

  return 1;
}
int syncIdMap_va(struct stored_conn_t *sconn, const char *table,
                 const char *primary_col, const char *map_col,
                 unsigned int primary_id, size_t n_maps, va_list args)
{
  unsigned int *map_ids;
  size_t idx;
  int ret_val;

  if ((map_ids = (unsigned int *)malloc(sizeof(unsigned int) * n_maps)) == 0) {
    return -1;
  }
  for (idx = 0; idx < n_maps; idx++) {
    *(map_ids + idx) = va_arg(args, unsigned int);
  }

  ret_val = syncIdMap(sconn, table, primary_col, map_col, primary_id, n_maps, map_ids);

  free(map_ids);

  return ret_val;
}


int hasIdMap(struct stored_conn_t *sconn, const char *table,
             const char *primary_col, const char *map_col,
             unsigned int primary_id, unsigned int map_id)
{
  int64_t qry_ret;
  where_builder *wb;

  wb = whereAnd(
        where(table, primary_col, EQ, TYPE_ID, 1, primary_id),
        where(table, map_col, EQ, TYPE_ID, 1, map_id)
        );
  qry_ret = countQuery(sconn, table, wb);
  free(wb);

  return qry_ret > 0;
}
int addIdMap(struct stored_conn_t *sconn, const char *table,
             const char *primary_col, const char *map_col,
             unsigned int primary_id, unsigned int map_id)
{
  str_builder *sb;
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return -1;
  }
  strbld_str(sb, "INSERT INTO `", 0);
  strbld_str(sb, table, 0);
  strbld_str(sb, "` (`", 0);
  strbld_str(sb, primary_col, 0);
  strbld_str(sb, "`, `", 0);
  strbld_str(sb, map_col, 0);
  strbld_str(sb, "`) VALUES ", 0);
  strbld_char(sb, '(');
  db_value_sb(sb, TYPE_ID, 1, primary_id);
  strbld_char(sb, ',');
  db_value_sb(sb, TYPE_ID, 1, map_id);
  strbld_char(sb, ')');
  strbld_str(sb, "` ON DUPLICATE KEY UPDATE `", 0);
  strbld_str(sb, primary_col, 0);
  strbld_str(sb, "` = ", 0);
  db_value_sb(sb, TYPE_ID, 1, primary_id);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return -1;
  }

  qry_ret = simpleQuery(sconn, qry, qry_len);
  free(qry);

  if (qry_ret == (uint64_t)-1) {
    return -1;
  }

  return 1;
}
int removeIdMap(struct stored_conn_t *sconn, const char *table,
                const char *primary_col, const char *map_col,
                unsigned int primary_id, unsigned int map_id)
{
  int qry_ret;
  where_builder *wb;

  wb = whereAnd(
        where(table, primary_col, EQ, TYPE_ID, 1, primary_id),
        where(table, map_col, EQ, TYPE_ID, 1, map_id)
        );
  qry_ret = deleteQuery(sconn, table, wb);
  free(wb);

  return qry_ret;
}
