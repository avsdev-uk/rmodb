#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "modb_metadata.h"
#include "modb_objects.h"
#include "modb_metadata_ext.h"
#include "modb_users.h"
#include "modb_groups.h"
#include "modb_p.h"


// ##### PRIVATE
int tableRowsToMetadataList(column_data **col_data, size_t n_cols,
                            struct metadata_t ***metadata_list, size_t *n_metadatas)
{
  column_data *col_id, *col_title, *col_owner_id, *col_created, *col_updated, *col_deleted;
  size_t n_rows, idx;
  struct metadata_t *metadata;

  n_rows = (*col_data)->n_values;
  if (n_rows == 0) {
    return 0;
  }

  col_id = findColumnByName(col_data, n_cols, "mdo_id");
  col_title = findColumnByName(col_data, n_cols, "title");
  col_owner_id = findColumnByName(col_data, n_cols, "owner_id");
  col_created = findColumnByName(col_data, n_cols, "created");
  col_updated = findColumnByName(col_data, n_cols, "updated");
  col_deleted = findColumnByName(col_data, n_cols, "deleted");

  *metadata_list = (struct metadata_t **)malloc(sizeof(struct metadata_t *) * n_rows);
  if (*metadata_list == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return -errno;
  }
  memset(*metadata_list, 0, sizeof(struct metadata_t *) * n_rows);

  for (idx = 0; idx < n_rows; idx++) {
    metadata = allocMetadata();
    if (metadata == 0) {
      freeMetadataList(metadata_list, idx - 1);
      return -1;
    }

    metadata->id = *(col_id->data.ptr_uint32 + idx);

    if (!moveColumnStrPointer(col_title, idx, 1, &metadata->title, &metadata->title_len)) {
      freeMetadataList(metadata_list, idx);
      return -1;
    }

    metadata->owner_id = *(col_owner_id->data.ptr_uint32 + idx);

    metadata->created_on = *(col_created->data.ptr_int64 + idx);
    if (!columnRowIsNull(col_updated, idx)) {
      metadata->updated_on = *(col_updated->data.ptr_int64 + idx);
    }
    if (!columnRowIsNull(col_deleted, idx)) {
      metadata->deleted_on = *(col_deleted->data.ptr_int64 + idx);
    }

    *(*metadata_list + idx) = metadata;
  }

  *n_metadatas = n_rows;

  return 1;
}
int doMetadataListQuery(stored_conn *sconn, modb_ref *modb, where_builder *wb,
                        struct metadata_t ***metadata_list, size_t *n_metadatas)
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
  modbTableName(&table, &table_len, modb, METADATA_TABLE, strlen(METADATA_TABLE));

  strbld_str(sb, "SELECT * FROM ", 0);
  escapeTableName_sb(sb, table, table_len);
  if (wb != 0) {
    compileWhereBuilder_sb(sb, wb, 0);
  }
  strbld_str(sb, " ORDER BY `updated` DESC, `created` DESC", 0);

  modbFreeTableName(&table);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return -1;
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

  res = tableRowsToMetadataList(col_data, n_cols, metadata_list, n_metadatas);
  freeColumns(col_data, n_cols);

  if (res <= 0) {
    return res;
  }

  return (int)qry_ret;
}
int doScalarMetadataListQuery(stored_conn *sconn, modb_ref *modb,
                              where_builder *wb, struct metadata_t **metadata)
{
  int res;
  struct metadata_t **metadata_list;
  size_t n_metadatas;

  res = doMetadataListQuery(sconn, modb, wb, &metadata_list, &n_metadatas);
  if (res <= 0) {
    return res;
  }

  *metadata = *(metadata_list + 0);
  *(metadata_list + 0) = 0;
  freeMetadataList(&metadata_list, n_metadatas);

  return res;
}



// ##### PUBLIC
struct metadata_t *allocMetadata(void)
{
  struct metadata_t *metadata;

  metadata = malloc(sizeof(struct metadata_t));
  if (metadata == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
  }
  memset(metadata, 0, sizeof(struct metadata_t));

  return metadata;
}
struct metadata_t **allocMetadataList(size_t n_metadatas)
{
  struct metadata_t **metadatas;

  metadatas = (struct metadata_t **)malloc(sizeof(struct metadata_t *) * n_metadatas);
  if (metadatas == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }
  memset(metadatas, 0, sizeof(struct metadata_t *) * n_metadatas);

  return metadatas;
}
void freeMetadata(struct metadata_t **metadata_ptr)
{
  struct metadata_t *metadata = *metadata_ptr;

  if (metadata->ext != 0) {
    freeMetadataExt(&metadata->ext);
  }

  if (metadata->object != 0) {
    freeObject(&metadata->object);
  }

  if (metadata->n_groups > 0) {
    if (metadata->group_ids != 0) {
      free(metadata->group_ids);
      metadata->group_ids = 0;
    }
    if (metadata->groups != 0) {
      freeGroups(&metadata->groups, metadata->n_groups);
    }
    metadata->n_groups = 0;
  }

  if (metadata->owner != 0) {
    freeUser(&metadata->owner);
  }

  if (metadata->title != 0) {
    free(metadata->title);
    metadata->title = 0;
  }

  free(metadata);
  *metadata_ptr = 0;
}
void freeMetadataList(struct metadata_t ***metadata_list_ptr, size_t n_metadatas)
{
  size_t idx;
  struct metadata_t **metadatas_list= *metadata_list_ptr;

  for (idx = 0; idx < n_metadatas; idx++) {
    if (*(metadatas_list + idx) != 0) {
      freeMetadata((metadatas_list + idx));
    }
  }

  free(metadatas_list);
  *metadata_list_ptr = 0;
}


int modbMetadataById(stored_conn *sconn, modb_ref *modb, unsigned int id,
                     struct metadata_t **metadata)
{
  where_builder *wb = 0;
  int res;

  wb = where(0, "mdo_id", EQ, TYPE_ID, 1, id);
  res = doScalarMetadataListQuery(sconn, modb, wb, metadata);
  freeWhereBuilder(&wb);

  return res;
}


int modbMetadataListByOwnerId(stored_conn *sconn, modb_ref *modb, unsigned int owner_id,
                              int with_deleted,
                              struct metadata_t ***metadata_list, size_t *n_metadatas)
{
  where_builder *wb = 0;
  int res;

  wb = where(0, "owner_id", EQ, TYPE_ID, owner_id);
  if (with_deleted == 0) {
    wb = whereAnd(wb, where(0, "deleted_on", IS_NULL, TYPE_RAW, 0, 0));
  }
  res = doMetadataListQuery(sconn, modb, wb, metadata_list, n_metadatas);
  if (wb != 0) {
    freeWhereBuilder(&wb);
  }

  return res;
}
int modbMetadataListByGroupId(stored_conn *sconn, modb_ref *modb, unsigned int group_id,
                              int with_deleted,
                              struct metadata_t ***metadata_list, size_t *n_metadatas)
{
  unsigned int *ids = 0;
  size_t n_ids;
  size_t idx;
  where_builder *wb = 0;
  int res;

  if (modbFetchGroupMetadataIds(sconn, modb, group_id, with_deleted, &ids, &n_ids) < 0) {
    return -1;
  }
  if (ids == 0) {
    return 0;
  }

  wb = whereIn(0, 0, "mdo_id", TYPE_ID, 0);
  for (idx = 0; idx < n_ids; idx++) {
    setWhereValue(wb, TYPE_ID, 1, *(ids + idx));
  }
  res = doMetadataListQuery(sconn, modb, wb, metadata_list, n_metadatas);
  if (wb != 0) {
    freeWhereBuilder(&wb);
  }

  free(ids);
  return res;
}

int modbMetadataList(stored_conn *sconn, modb_ref *modb, int with_deleted,
                     struct metadata_t ***metadata_list, size_t *n_metadatas)
{
  where_builder *wb = 0;
  int res;

  if (with_deleted == 0) {
    wb = where(0, "deleted_on", IS_NULL, TYPE_RAW, 0, 0);
  }
  res = doMetadataListQuery(sconn, modb, wb, metadata_list, n_metadatas);
  if (wb != 0) {
    freeWhereBuilder(&wb);
  }

  return res;
}

int64_t modbMetadataCreate(stored_conn *sconn, modb_ref *modb,
                           const struct metadata_t *const metadata)
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
  modbTableName(&table, &table_len, modb, METADATA_TABLE, strlen(METADATA_TABLE));

  strbld_str(sb, "INSERT INTO ", 0);
  escapeTableName_sb(sb, table, table_len);
  strbld_str(sb, " (`mdo_id`, `type`, `title`, `owner_id`) VALUES (", 0);
  db_value_sb(sb, TYPE_ID, 1, metadata->id);
  strbld_char(sb, ',');
  db_value_sb(sb, TYPE_STRING, 2, metadata->type, metadata->type_len);
  strbld_char(sb, ',');
  db_value_sb(sb, TYPE_STRING, 2, metadata->title, metadata->title_len);
  strbld_char(sb, ',');
  if (metadata->owner != 0) {
    db_value_sb(sb, TYPE_ID, 1, metadata->owner->id);
  } else {
    db_value_sb(sb, TYPE_ID, 1, metadata->owner_id);
  }
  strbld_char(sb, ')');

  modbFreeTableName(&table);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return -1;
  }

  qry_ret = simpleQuery(sconn, qry, qry_len);
  free(qry);

  return (int64_t)qry_ret;
}

int64_t modbMetadataReplace(stored_conn *sconn, modb_ref *modb, unsigned int id,
                            const struct metadata_t *const metadata)
{
  str_builder *sb;
  char *table, *set;
  size_t table_len, set_len;
  int64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  if (metadata->id != 0 && metadata->id != id) {
    strbld_char(sb, ',');
    columnSetValueStr_sb(sb, "mdo_id", TYPE_ID, 1, metadata->id);
  }
  if (metadata->type != 0) {
    strbld_char(sb, ',');
    columnSetValueStr_sb(sb, "type", TYPE_STRING, 2, metadata->type, metadata->type_len);
  }
  if (metadata->title != 0) {
    strbld_char(sb, ',');
    columnSetValueStr_sb(sb, "title", TYPE_STRING, 2, metadata->title, metadata->title_len);
  }
  if (metadata->owner != 0) {
    strbld_char(sb, ',');
    columnSetValueStr_sb(sb, "owner_id", TYPE_ID, 1, metadata->owner->id);
  } else if (metadata->owner_id != 0) {
    strbld_char(sb, ',');
    columnSetValueStr_sb(sb, "owner_id", TYPE_ID, 1, metadata->owner_id);
  }
  if (strbld_finalize_or_destroy(&sb, &set, &set_len) != 0) {
    return 0;
  }

  modbTableName(&table, &table_len, modb, METADATA_TABLE, STR_LEN(METADATA_TABLE));
  qry_ret = updateQuery(
        sconn, table, table_len, set, set_len, where(0, "mdo_id", EQ, TYPE_ID, 1, id)
        );
  modbFreeTableName(&table);

  free(set);

  return qry_ret;
}
int64_t modbMetadataUpdateType(stored_conn *sconn, modb_ref *modb, unsigned int id,
                               const char *type, size_t type_len)
{
  str_builder *sb;
  char *table, *set;
  size_t table_len, set_len;
  int64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  columnSetValueStr_sb(sb, "mdo_id", TYPE_ID, 1, id);
  strbld_char(sb, ',');
  columnSetValueStr(&set, &set_len, "type", TYPE_STRING, 2, type, type_len);
  if (strbld_finalize_or_destroy(&sb, &set, &set_len) != 0) {
    return 0;
  }

  modbTableName(&table, &table_len, modb, METADATA_TABLE, STR_LEN(METADATA_TABLE));
  qry_ret = updateQuery(
        sconn, table, table_len, set, set_len, where(0, "mdo_id", EQ, TYPE_ID, 1, id)
        );
  modbFreeTableName(&table);

  free(set);

  return qry_ret;
}
int64_t modbMetadataUpdateTitle(stored_conn *sconn, modb_ref *modb, unsigned int id,
                                const char *title, size_t title_len)
{
  str_builder *sb;
  char *table, *set;
  size_t table_len, set_len;
  int64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  columnSetValueStr_sb(sb, "mdo_id", TYPE_ID, 1, id);
  strbld_char(sb, ',');
  columnSetValueStr(&set, &set_len, "title", TYPE_STRING, 2, title, title_len);
  if (strbld_finalize_or_destroy(&sb, &set, &set_len) != 0) {
    return 0;
  }

  modbTableName(&table, &table_len, modb, METADATA_TABLE, STR_LEN(METADATA_TABLE));
  qry_ret = updateQuery(
        sconn, table, table_len, set, set_len, where(0, "mdo_id", EQ, TYPE_ID, 1, id)
        );
  modbFreeTableName(&table);

  free(set);

  return qry_ret;
}
int64_t modbMetadataUpdateOwner(stored_conn *sconn, modb_ref *modb, unsigned int id,
                                struct user_t *owner)
{
  return modbMetadataUpdateOwnerId(sconn, modb, id, owner->id);
}
int64_t modbMetadataUpdateOwnerId(stored_conn *sconn, modb_ref *modb, unsigned int id,
                                  unsigned int owner_id)
{
  str_builder *sb;
  char *table, *set;
  size_t table_len, set_len;
  int64_t qry_ret;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  columnSetValueStr_sb(sb, "mdo_id", TYPE_ID, 1, id);
  strbld_char(sb, ',');
  columnSetValueStr(&set, &set_len, "owner_id", TYPE_ID, 1, owner_id);
  if (strbld_finalize_or_destroy(&sb, &set, &set_len) != 0) {
    return 0;
  }

  modbTableName(&table, &table_len, modb, METADATA_TABLE, STR_LEN(METADATA_TABLE));
  qry_ret = updateQuery(
        sconn, table, table_len, set, set_len, where(0, "mdo_id", EQ, TYPE_ID, 1, id)
        );
  modbFreeTableName(&table);

  free(set);

  return qry_ret;
}

int modbMetadataDelete(stored_conn *sconn, modb_ref *modb, unsigned int id)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, METADATA_TABLE, STR_LEN(METADATA_TABLE));
  qry_ret = softDeleteByIdQuery(sconn, table, table_len, "id", id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}
int modbMetadataDestroy(stored_conn *sconn, modb_ref *modb, unsigned int id)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, METADATA_TABLE, STR_LEN(METADATA_TABLE));
  qry_ret = deleteByIdQuery(sconn, table, table_len, "id", id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}


// MODB Metadata -> Owner
int64_t modbFetchMetadataOwner(stored_conn *sconn, modb_ref *modb, struct metadata_t *metadata)
{
  return modbUserById(sconn, modb, metadata->owner_id, 0, &metadata->owner);
}

// MODB Metadata -> Object
int64_t modbFetchMetadataObject(stored_conn *sconn, modb_ref *modb, struct metadata_t *metadata)
{
  return modbObjectById(sconn, modb, metadata->id, &metadata->object);
}

// MODB Metadata -> MetadataExtended
int64_t modbFetchMetadataExtended(stored_conn *sconn, modb_ref *modb, struct metadata_t *metadata)
{
  // TODO: implement
  return -1;
}

// MODB Metadata -> Groups
int modbFetchMetadataGroupIds(stored_conn *sconn, modb_ref *modb,
                              struct metadata_t *metadata, int with_deleted)
{
  char *g_table, *mg_table;
  size_t g_len, mg_len;
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;

  str_builder *sb;
  where_builder *wb;

  column_data **col_data;
  size_t n_cols;

  if ((sb = strbld_create()) == 0) {
    return -1;
  }
  modbTableName(&g_table, &g_len, modb, GROUPS_TABLE, STR_LEN(GROUPS_TABLE));
  modbTableName(&mg_table, &mg_len, modb, MDO_GROUPS_TABLE, STR_LEN(MDO_GROUPS_TABLE));

  strbld_str(sb, "SELECT ", 0);
  escapeColumnName_sb(sb, 0, 0, "group_id", 0);
  strbld_str(sb, " FROM ", 0);
  escapeTableName_sb(sb, mg_table, mg_len);
  joinStr_sb(sb, " LEFT", 5, 1, g_table, g_len, "id", 2, mg_table, mg_len, "group_id", 8);
  wb = where(0, "mdo_id", EQ, TYPE_ID, 1, metadata->id);
  if (!with_deleted) {
    wb = whereAnd(wb, where(g_table, "deleted", IS_NULL, TYPE_RAW, 0));
  }
  compileWhereBuilder_sb(sb, wb, 1);

  modbFreeTableName(&g_table);
  modbFreeTableName(&mg_table);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return -1;
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

  metadata->group_ids = (*col_data)->data.ptr_uint32;
  metadata->n_groups = (*col_data)->n_values;
  (*col_data)->data.ptr_uint32 = 0;

  freeColumns(col_data, n_cols);

  return 1;
}
int modbFetchMetadataGroups(stored_conn *sconn, modb_ref *modb, struct metadata_t *metadata,
                            int with_deleted)
{
  int ret;
  size_t idx;

  ret = modbFetchMetadataGroupIds(sconn, modb, metadata, with_deleted);
  if (ret != 1) {
    return ret;
  }

  metadata->groups = allocGroups(metadata->n_groups);
  if (metadata->groups == 0) {
    return -1;
  }

  for (idx = 0; idx < metadata->n_groups; idx++) {
    if (modbGroupById(
          sconn, modb, *(metadata->group_ids + idx), 0, (metadata->groups + idx)
          ) != 1) {
      freeGroups(&metadata->groups, metadata->n_groups);
      return -1;
    }
  }

  return ret;
}


int modbSyncMetadataGroups(stored_conn *sconn, modb_ref *modb,
                           unsigned int metadata_id, size_t n_groups, unsigned int *group_ids)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, MDO_GROUPS_TABLE, STR_LEN(MDO_GROUPS_TABLE));
  qry_ret = syncIdMap(
        sconn, table, table_len, "mdo_id", "group_id", metadata_id, n_groups, group_ids
        );
  modbFreeTableName(&table);

  return (int)qry_ret;
}
int modbSyncMetadataGroups_va(stored_conn *sconn, modb_ref *modb,
                              unsigned int metadata_id, size_t n_groups, ...)
{
  va_list args;
  char *table;
  size_t table_len;
  int64_t qry_ret;

  va_start(args, n_groups);
  modbTableName(&table, &table_len, modb, MDO_GROUPS_TABLE, STR_LEN(MDO_GROUPS_TABLE));
  qry_ret = syncIdMap_va(
        sconn, table, table_len, "mdo_id", "group_id", metadata_id, n_groups, args
        );
  modbFreeTableName(&table);
  va_end(args);

  return (int)qry_ret;
}

int modbIsLinked_Metadata_Group(stored_conn *sconn, modb_ref *modb,
                                unsigned int metadata_id, unsigned int group_id)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, MDO_GROUPS_TABLE, STR_LEN(MDO_GROUPS_TABLE));
  qry_ret = hasIdMap(sconn, table, table_len, "mdo_id", "group_id", metadata_id, group_id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}
int modbLink_Metadata_Group(stored_conn *sconn, modb_ref *modb,
                            unsigned int metadata_id, unsigned int group_id)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, MDO_GROUPS_TABLE, STR_LEN(MDO_GROUPS_TABLE));
  qry_ret = addIdMap(sconn, table, table_len, "mdo_id", "group_id", metadata_id, group_id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}
int modbUnlink_Metadata_Group(stored_conn *sconn, modb_ref *modb,
                              unsigned int metadata_id, unsigned int group_id)
{
  char *table;
  size_t table_len;
  int64_t qry_ret;

  modbTableName(&table, &table_len, modb, MDO_GROUPS_TABLE, STR_LEN(MDO_GROUPS_TABLE));
  qry_ret = removeIdMap(sconn, table, table_len, "mdo_id", "group_id", metadata_id, group_id);
  modbFreeTableName(&table);

  return (int)qry_ret;
}


// MODB Group -> Metadatas
int modbFetchGroupMetadataIds(stored_conn *sconn, modb_ref *modb,
                              unsigned int group_id, int with_deleted,
                              unsigned int **metadata_ids, size_t *n_ids)
{
  char *mg_table, *m_table;
  size_t mg_len, m_len;
  char *qry;
  size_t qry_len;
  uint64_t qry_ret;

  str_builder *sb;
  where_builder *wb;

  column_data **col_data;
  size_t n_cols;

  if ((sb = strbld_create()) == 0) {
    return -1;
  }
  modbTableName(&m_table, &m_len, modb, METADATA_TABLE, STR_LEN(METADATA_TABLE));
  modbTableName(&mg_table, &mg_len, modb, MDO_GROUPS_TABLE, STR_LEN(MDO_GROUPS_TABLE));

  strbld_str(sb, "SELECT ", 7);
  escapeColumnName_sb(sb, mg_table, mg_len, "mdo_id", 6);
  strbld_str(sb, " FROM ", 6);
  escapeTableName_sb(sb, mg_table, mg_len);
  joinStr_sb(sb, " LEFT", 5, 1, m_table, m_len, "mdo_id", 6, mg_table, mg_len, "mdo_id", 6);
  wb = where(0, "group_id", EQ, TYPE_ID, 1, group_id);
  if (!with_deleted) {
    wb = whereAnd(wb, where(m_table, "deleted", IS_NULL, TYPE_RAW, 0));
  }
  compileWhereBuilder_sb(sb, wb, 1);

  modbFreeTableName(&mg_table);
  modbFreeTableName(&m_table);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return -1;
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

  *metadata_ids = (*col_data)->data.ptr_uint32;
  *n_ids = (*col_data)->n_values;
  (*col_data)->data.ptr_uint32 = 0;

  freeColumns(col_data, n_cols);

  return 1;
}
