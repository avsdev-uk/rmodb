#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "modb_manage_p.h"
#include "modb_p.h"
#include "strext.h"


struct sconn_modb_use_t {
  struct stored_conn_t *sconn;
  char *modb_name;
  size_t modb_name_len;

  struct sconn_modb_use_t *next;
  struct sconn_modb_use_t *prev;
};

static struct sconn_modb_use_t *storedUses = 0;


uint64_t createSysTable(struct stored_conn_t *sconn, struct modb_t *modb)
{
  char *qry;
  uint64_t res;
  size_t qry_len;
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return (uint64_t)-1;
  }
  strbld_str(sb, "CREATE TABLE `", 0);
  modbTableName_sb(sb, modb, SYS_TABLE, STR_LEN(SYS_TABLE));
  strbld_str(sb, "` ("
                 "`key` VARCHAR(255) NULL, "
                 "`value` VARCHAR(255) NULL, "
                 "UNIQUE(`key`)"
                 ")", 0);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return (uint64_t)-1;
  }

  res = simpleQuery(sconn, qry, qry_len);
  free(qry);

  return res;
}
uint64_t createMetaTable(struct stored_conn_t *sconn, struct modb_t *modb)
{
  char *qry;
  uint64_t res;
  size_t qry_len;
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return (uint64_t)-1;
  }
  strbld_str(sb, "CREATE TABLE `", 0);
  modbTableName_sb(sb, modb, META_TABLE, STR_LEN(META_TABLE));
  strbld_str(sb, "` ("
                 "`mdo_id` INT UNSIGNED NOT NULL AUTO_INCREMENT, "
                 "`title` VARCHAR(255) NOT NULL, "
                 "`owner` INT UNSIGNED NOT NULL, "
                 "`created` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, "
                 "`updated` TIMESTAMP on update CURRENT_TIMESTAMP NULL DEFAULT NULL, "
                 "`deleted` TIMESTAMP NULL DEFAULT NULL, "
                 "PRIMARY KEY(`mdo_id`)"
                 ")", 0);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return (uint64_t)-1;
  }

  res = simpleQuery(sconn, qry, qry_len);
  free(qry);

  return res;
}
uint64_t createObjectsTable(struct stored_conn_t *sconn, struct modb_t *modb)
{
  char *qry;
  uint64_t res;
  size_t qry_len;
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return (uint64_t)-1;
  }
  strbld_str(sb, "CREATE TABLE `", 0);
  modbTableName_sb(sb, modb, OBJECTS_TABLE, STR_LEN(OBJECTS_TABLE));
  strbld_str(sb, "` ("
                 "`mdo_id` INT UNSIGNED NOT NULL, "
                 "`object` MEDIUMBLOB NOT NULL, "
                 "PRIMARY KEY (`mdo_id`)"
                 ")", 0);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return (uint64_t)-1;
  }

  res = simpleQuery(sconn, qry, qry_len);
  free(qry);

  return res;
}
uint64_t createMDOGroupsTable(struct stored_conn_t *sconn, struct modb_t *modb)
{
  char *qry;
  uint64_t res;
  size_t qry_len;
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return (uint64_t)-1;
  }
  strbld_str(sb, "CREATE TABLE `", 0);
  modbTableName_sb(sb, modb, MDO_GROUPS_TABLE, STR_LEN(MDO_GROUPS_TABLE));
  strbld_str(sb, "` ("
                 "`mdo_id` INT UNSIGNED NOT NULL, "
                 "`group_id` INT UNSIGNED NOT NULL, "
                 "INDEX(`mdo_id`), "
                 "INDEX(`group_id`)"
                 ")", 0);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return (uint64_t)-1;
  }

  res = simpleQuery(sconn, qry, qry_len);
  free(qry);

  return res;
}

uint64_t createUsersTable(struct stored_conn_t *sconn, struct modb_t *modb)
{
  char *qry;
  uint64_t res;
  size_t qry_len;
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return (uint64_t)-1;
  }
  strbld_str(sb, "CREATE TABLE `", 0);
  modbTableName_sb(sb, modb, USERS_TABLE, STR_LEN(USERS_TABLE));
  strbld_str(sb, "` ("
                 "`id` INT UNSIGNED NOT NULL AUTO_INCREMENT, "
                 "`username` VARCHAR(255) NOT NULL, "
                 "`email` VARCHAR(255) NOT NULL, "
                 "`created` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, "
                 "`updated` TIMESTAMP on update CURRENT_TIMESTAMP NULL DEFAULT NULL, "
                 "`deleted` TIMESTAMP NULL DEFAULT NULL, "
                 "INDEX (`id`)"
                 ")", 0);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return (uint64_t)-1;
  }

  res = simpleQuery(sconn, qry, qry_len);
  free(qry);

  return res;
}
uint64_t createGroupsTable(struct stored_conn_t *sconn, struct modb_t *modb)
{
  char *qry;
  uint64_t res;
  size_t qry_len;
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return (uint64_t)-1;
  }
  strbld_str(sb, "CREATE TABLE `", 0);
  modbTableName_sb(sb, modb, GROUPS_TABLE, STR_LEN(GROUPS_TABLE));
  strbld_str(sb, "` ("
                 "`id` INT UNSIGNED NOT NULL AUTO_INCREMENT, "
                 "`name` VARCHAR(255) NOT NULL, "
                 "`created` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, "
                 "`updated` TIMESTAMP on update CURRENT_TIMESTAMP NULL DEFAULT NULL, "
                 "`deleted` TIMESTAMP NULL DEFAULT NULL, "
                 "INDEX (`id`)"
                 ")", 0);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return (uint64_t)-1;
  }

  res = simpleQuery(sconn, qry, qry_len);
  free(qry);

  return res;
}
uint64_t createUserGroupsTable(struct stored_conn_t *sconn, struct modb_t *modb)
{
  char *qry;
  uint64_t res;
  size_t qry_len;
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return (uint64_t)-1;
  }
  strbld_str(sb, "CREATE TABLE `", 0);
  modbTableName_sb(sb, modb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE));
  strbld_str(sb, "` ("
                 "`user_id` INT UNSIGNED NOT NULL, "
                 "`group_id` INT UNSIGNED NOT NULL, "
                 "INDEX(`user_id`), "
                 "INDEX(`group_id`)"
                 ")", 0);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return (uint64_t)-1;
  }

  res = simpleQuery(sconn, qry, qry_len);
  free(qry);

  return res;
}


char *createColString(struct column_data_t *col)
{
  char *colstr;
  size_t colstr_len;
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  strbld_str(sb, ", `", 3);
  strbld_str(sb, col->name, col->name_len);
  strbld_str(sb, "` ", 2);

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

  if (strbld_finalize_or_destroy(&sb, &colstr, &colstr_len) != 0) {
    return 0;
  }

  return colstr;
}
uint64_t createMetaExtTable(struct stored_conn_t *sconn, struct modb_t *modb,
                            struct column_data_t **col_data, size_t cols)
{
  char *qry;
  uint64_t res;
  size_t qry_len;
  str_builder *sb;
  char *colstr;

  if ((sb = strbld_create()) == 0) {
    return (uint64_t)-1;
  }
  strbld_str(sb, "CREATE TABLE `", 0);
  modbTableName_sb(sb, modb, META_EXT_TABLE, STR_LEN(META_EXT_TABLE));
  strbld_str(sb, "` ("
                 "`mdo_id` INT UNSIGNED NOT NULL", 0);
  for (size_t c = 0; c < cols; c++) {
    struct column_data_t *col = *(col_data + c);
    if ((colstr = createColString(col)) == 0) {
      strbld_destroy(&sb);
      return (uint64_t)-1;
    }
    strbld_str(sb, colstr, 0);
    free(colstr);
  }
  strbld_str(sb, ", INDEX (`mdo_id`))", 0);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return (uint64_t)-1;
  }

  res = simpleQuery(sconn, qry, qry_len);
  free(qry);

  return res;
}


int tableExists(struct stored_conn_t *sconn, struct modb_t *modb,
                const char *suffix, size_t suffix_len)
{
  char *qry, *res;
  size_t qry_len;
  str_builder *sb;
  int retval = 0;

  if ((sb = strbld_create()) == 0) {
    return -errno;
  }
  strbld_str(sb, "SHOW TABLES LIKE '", 0);
  modbTableName_sb(sb, modb, suffix, suffix_len);
  strbld_char(sb, '\'');
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return -errno;
  }

  res = scalarString(sconn, qry, qry_len, "Z");
  /* Result:
   * 0: query success, no result returned
   * 1: query failed, default ("Z") returned
   * 2: query success, result does not match required length
   * 3: query success, result matches required length (check content)
   */
  if (res == 0) {
    retval = 0;
  } else if (strlen(res) == 1 && strncmp(res, "Z", 1) == 0) {
    retval = -1;
  } else if (strlen(res) != (modb->name_len + suffix_len)) {
    retval = 0;
  } else {
    retval = strncmp(res, modb->name, modb->name_len) == 0;
  }

  if (res != 0) {
    free(res);
  }
  free(qry);

  return retval;
}


uint64_t destroyTable(struct stored_conn_t *sconn, struct modb_t *modb,
                      const char *suffix, size_t suffix_len)
{
  char *qry;
  uint64_t res;
  size_t qry_len;
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return (uint64_t)-1;
  }
  strbld_str(sb, "DROP TABLE `", 0);
  modbTableName_sb(sb, modb, suffix, suffix_len);
  strbld_char(sb, '`');
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return (uint64_t)-1;
  }

  res = simpleQuery(sconn, qry, qry_len);
  free(qry);

  return res;
}


struct sconn_modb_use_t *allocUse(struct stored_conn_t *sconn, struct modb_t *modb)
{
  struct sconn_modb_use_t *ptr = 0;
  struct sconn_modb_use_t *tail;

  ptr = (struct sconn_modb_use_t *)malloc(sizeof(struct sconn_modb_use_t));
  if (ptr == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }
  memset(ptr, 0, sizeof(struct sconn_modb_use_t));

  ptr->sconn = sconn;
  if (strmemcpy(modb->name, modb->name_len, &ptr->modb_name, &ptr->modb_name_len) != 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    free(ptr);
    return 0;
  }

  if (storedUses == 0) {
    storedUses = ptr;
  } else {
    tail = storedUses;
    while (tail->next != 0) {
      tail = tail->next;
    }
    tail->next = ptr;
    ptr->prev = tail;
  }

  return ptr;
}
void freeUse(struct sconn_modb_use_t *ptr)
{
  if (ptr->prev) {
    ptr->prev->next = ptr->next;
  } else {
    storedUses = ptr->next;
  }

  if (ptr->next) {
    ptr->next->prev = ptr->prev;
  }

  free(ptr->modb_name);
  free(ptr);
}

int connectionUseMODB(struct stored_conn_t *sconn, struct modb_t *modb, int override)
{
  struct sconn_modb_use_t *ptr = storedUses;
  char *old_name;

  while (ptr != 0) {
    if (ptr->sconn == sconn) {
      break;
    }
    ptr = ptr->next;
  }

  if (ptr != 0) {
    if (ptr->modb_name_len != modb->name_len
        && strncmp(ptr->modb_name, modb->name, modb->name_len) == 0) {
      return 0;
    } else {
      if (override == 0) {
        fprintf(
              stderr,
              "[%d]useMODB: Connection is already using MODB '%s'\n",
              __LINE__, ptr->modb_name
              );
        return -1;
      }

      old_name = ptr->modb_name;
      ptr->modb_name = (char *)realloc(ptr->modb_name, modb->name_len);
      if (ptr->modb_name == 0) {
        fprintf(stderr, "[%d]realloc: (%d) %s\n", __LINE__, errno, strerror(errno));
        ptr->modb_name = old_name;
        return -1;
      }
      memcpy(ptr->modb_name, modb->name, modb->name_len);
      return 0;
    }
  }

  if (allocUse(sconn, modb) == 0) {
    return -1;
  }

  return 0;
}
int connectionGetUse(struct stored_conn_t *sconn, struct modb_t *modb)
{
  struct sconn_modb_use_t *ptr = storedUses;

  while (ptr != 0) {
    if (ptr->sconn == sconn) {
      if (modb != 0) {
        modb->name = ptr->modb_name;
        modb->name_len = ptr->modb_name_len;
        return 1;
      }
    }
    ptr = ptr->next;
  }

  return 0;
}
void connectionReleaseMODB(struct stored_conn_t *sconn)
{
  struct sconn_modb_use_t *ptr = storedUses;

  while (ptr != 0) {
    if (ptr->sconn == sconn) {
      break;
    }
    ptr = ptr->next;
  }

  if (ptr == 0) {
    return;
  }

  freeUse(ptr);
}
