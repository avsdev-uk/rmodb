#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "modb_objects.h"
#include "modb_p.h"


int tableRowsToObjects(column_data **col_data, size_t n_cols,
                       struct object_t ***objects, size_t *n_objects)
{
  column_data *col_id, *col_object;
  size_t n_rows, idx;
  struct object_t *object;

  n_rows = (*col_data)->n_values;
  if (n_rows == 0) {
    return 0;
  }

  col_id = findColumnByName(col_data, n_cols, "mdo_id");
  col_object = findColumnByName(col_data, n_cols, "object");

  *objects = (struct object_t **)malloc(sizeof(struct object_t *) * n_rows);
  if (*objects == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return -1;
  }
  memset(*objects, 0, sizeof(struct object_t *) * n_rows);

  for (idx = 0; idx < n_rows; idx++) {
    object = allocObject();
    if (object == 0) {
      freeObjects(objects, idx - 1);
      return -1;
    }

    object->id = *(col_id->data.ptr_uint32 + idx);

    if (!moveColumnBlobPointer(col_object, idx, 1, &object->data, &object->data_len)) {
      freeObjects(objects, idx);
      return -1;
    }

    *(*objects + idx) = object;
  }

  *n_objects = n_rows;

  return 1;
}
int doObjectsQuery(stored_conn *sconn, modb_ref *modb, where_builder *wb,
                   struct object_t ***objects, size_t *n_objects)
{
  char *table;
  size_t table_len;
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;
  int res;

  str_builder *sb;

  column_data **col_data;
  size_t n_cols;


  if ((sb = strbld_create()) == 0) {
    return -1;
  }
  modbTableName(&table, &table_len, modb, OBJECTS_TABLE, strlen(OBJECTS_TABLE));

  strbld_str(sb, "SELECT * FROM ", 0);
  escapeTableName_sb(sb, table, table_len);
  if (wb != 0) {
    compileWhereBuilder_sb(sb, wb, 0);
  }

  modbFreeTableName(&table);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return -errno;
  }

  qry_ret = tableQuery(sconn, qry, qry_len, 0, &col_data, &n_cols);
  free(qry);

  // Query failed
  if (qry_ret == (uint64_t)-1) {
    return -1;
  }

  // Zero row result
  if (qry_ret == 0) {
    freeColumns(col_data, n_cols);
    return 0;
  }

  res = tableRowsToObjects(col_data, n_cols, objects, n_objects);
  freeColumns(col_data, n_cols);

  if (res <= 0) {
    return res;
  }

  return (int)qry_ret;
}
int doScalarObjectsQuery(stored_conn *sconn, modb_ref *modb,
                         where_builder *wb, struct object_t **object)
{
  int res;
  struct object_t **objects;
  size_t n_objects;

  res = doObjectsQuery(sconn, modb, wb, &objects, &n_objects);
  if (res <= 0) {
    return res;
  }

  *object = *(objects + 0);
  *(objects + 0) = 0;
  freeObjects(&objects, n_objects);

  return res;
}



int modbObjectById(stored_conn *sconn, modb_ref *modb, unsigned int id,
                   struct object_t **object)
{
  where_builder *wb = 0;
  int res;

  wb = where(0, "mdo_id", EQ, TYPE_UINT32, 1, id);
  res = doScalarObjectsQuery(sconn, modb, wb, object);
  freeWhereBuilder(&wb);

  return res;
}

int modbObjectList(stored_conn *sconn, modb_ref *modb, int with_deleted,
                   struct object_t ***objects, size_t *n_objects)
{
  where_builder *wb = 0;
  int res;

  if (with_deleted == 0) {
    wb = where(0, "deleted_on", IS_NULL, TYPE_RAW, 0, 0);
  }
  res = doObjectsQuery(sconn, modb, wb, objects, n_objects);
  if (wb != 0) {
    freeWhereBuilder(&wb);
  }

  return res;
}

int64_t modbObjectCreate(stored_conn *sconn, modb_ref *modb,
                         unsigned int id, const char *data, size_t data_len)
{
  str_builder *sb;
  char *table;
  size_t table_len;
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return -1;
  }
  modbTableName(&table, &table_len, modb, OBJECTS_TABLE, strlen(OBJECTS_TABLE));

  strbld_str(sb, "INSERT INTO ", 0);
  escapeTableName_sb(sb, table, table_len);
  strbld_str(sb, " (`mdo_id`, `object`) VALUES (", 0);
  db_value_sb(sb, TYPE_ID, 1, id);
  strbld_char(sb, ',');
  db_value_sb(sb, TYPE_BLOB, 2, data, data_len);
  strbld_char(sb, ')');

  modbFreeTableName(&table);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return -1;
  }

  qry_ret = simpleQuery(sconn, qry, qry_len);
  free(qry);

  return (int64_t)qry_ret;
}
int64_t modbObjectUpdate(stored_conn *sconn, modb_ref *modb, unsigned int id,
                         const char *data, size_t data_len)
{
  str_builder *sb;
  char *table, *set;
  size_t table_len, set_len;
  int64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  columnSetValueStr_sb(sb, "mdo_id", TYPE_ID, 1, id);
  if (data != 0) {
    strbld_char(sb, ',');
    columnSetValueStr_sb(sb, "object", TYPE_BLOB, 2, data, data_len);
  }
  if (strbld_finalize_or_destroy(&sb, &set, &set_len) != 0) {
    return 0;
  }

  modbTableName(&table, &table_len, modb, OBJECTS_TABLE, strlen(OBJECTS_TABLE));
  qry_ret = updateQuery(
        sconn, table, table_len, set, set_len, where(0, "mdo_id", EQ, TYPE_ID, 1, id)
        );
  modbFreeTableName(&table);

  free(set);

  return qry_ret;
}
int modbObjectDelete(stored_conn *sconn, modb_ref *modb, unsigned int id)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, OBJECTS_TABLE, STR_LEN(OBJECTS_TABLE));
  qry_ret = softDeleteByIdQuery(sconn, table, table_len, "id", id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}
int modbObjectDestroy(stored_conn *sconn, modb_ref *modb, unsigned int id)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, OBJECTS_TABLE, STR_LEN(OBJECTS_TABLE));
  qry_ret = deleteByIdQuery(sconn, table, table_len, "id", id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}
