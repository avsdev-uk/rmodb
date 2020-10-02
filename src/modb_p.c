#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "modb_p.h"
#include "db_query.h"
#include "strext.h"


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
  strbld_str(sb, modb->name, modb->name_len);
  strbld_str(sb, SYS_TABLE, STR_LEN(SYS_TABLE));
  strbld_str(sb, "` ", 2);
  strbld_str(sb, "("
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
  strbld_str(sb, modb->name, modb->name_len);
  strbld_str(sb, META_TABLE, STR_LEN(META_TABLE));
  strbld_str(sb, "` ", 2);
  strbld_str(sb, "("
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
  strbld_str(sb, modb->name, modb->name_len);
  strbld_str(sb, OBJECTS_TABLE, STR_LEN(OBJECTS_TABLE));
  strbld_str(sb, "` ", 2);
  strbld_str(sb, "("
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
  strbld_str(sb, modb->name, modb->name_len);
  strbld_str(sb, MDO_GROUPS_TABLE, STR_LEN(MDO_GROUPS_TABLE));
  strbld_str(sb, "` ", 2);
  strbld_str(sb, "("
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
  strbld_str(sb, modb->name, modb->name_len);
  strbld_str(sb, USERS_TABLE, STR_LEN(USERS_TABLE));
  strbld_str(sb, "` ", 2);
  strbld_str(sb, "("
                 "`id` INT UNSIGNED NOT NULL, "
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
  strbld_str(sb, modb->name, modb->name_len);
  strbld_str(sb, GROUPS_TABLE, STR_LEN(GROUPS_TABLE));
  strbld_str(sb, "` ", 2);
  strbld_str(sb, "("
                 "`id` INT UNSIGNED NOT NULL, "
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
  strbld_str(sb, modb->name, modb->name_len);
  strbld_str(sb, USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE));
  strbld_str(sb, "` ", 2);
  strbld_str(sb, "("
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
  retval += strbld_str(sb, "SHOW TABLES LIKE '", 0);
  retval += strbld_str(sb, modb->name, modb->name_len);
  retval += strbld_str(sb, suffix, suffix_len);
  retval += strbld_char(sb, '\'');
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return -errno;
  }

  res = scalarString(sconn, qry, qry_len, "Z");
  if (res == 0 || strlen(res) != (modb->name_len + suffix_len)) {
    retval = -1;
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
  strbld_str(sb, modb->name, modb->name_len);
  strbld_str(sb, suffix, suffix_len);
  strbld_str(sb, "` ", 2);
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
  }

  if (col->isUnsigned) {
    strbld_str(sb, " UNSIGNED", 0);
  }

  if (col->isNullable) {
    strbld_str(sb, " NULL", 0);
  } else {
    strbld_str(sb, " NOT NULL", 0);
  }

  if (strbld_finalize_or_destroy(&sb, &colstr, &colstr_len) != 0) {
    return 0;
  }

  return colstr;
}

