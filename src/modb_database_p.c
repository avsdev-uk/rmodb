#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "modb_database_p.h"
#include "modb_p.h"
#include "strext.h"


struct sconn_modb_use_t {
  stored_conn *sconn;
  char *modb_name;
  size_t modb_name_len;

  struct sconn_modb_use_t *next;
  struct sconn_modb_use_t *prev;
};

static struct sconn_modb_use_t *storedUses = 0;


struct sconn_modb_use_t *allocUse(stored_conn *sconn, modb_ref *modb)
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

int connectionUseMODB(stored_conn *sconn, modb_ref *modb, int override)
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
              "[%d]connectionUseMODB: Connection is already using MODB '%s'\n",
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
int connectionGetMODB(stored_conn *sconn, modb_ref *modb)
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
void connectionUnuseMODB(stored_conn *sconn)
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


uint64_t createMODBTable(stored_conn *sconn, modb_ref *modb, const char *suffix, size_t suffix_len,
                         const char *table_def, size_t table_def_len)
{
  char *table;
  size_t table_len;
  char *qry;
  uint64_t res;
  size_t qry_len;
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return (uint64_t)-1;
  }
  modbTableName(&table, &table_len, modb, suffix, suffix_len);

  strbld_str(sb, "CREATE TABLE ", 0);
  escapeTableName_sb(sb, table, table_len);
  strbld_char(sb, ' ');
  strbld_str(sb, table_def, table_def_len);

  modbFreeTableName(&table);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return (uint64_t)-1;
  }

  res = simpleQuery(sconn, qry, qry_len);
  free(qry);

  return res;
}

int MODBTableExists(stored_conn *sconn, modb_ref *modb, const char *suffix, size_t suffix_len)
{
  char *qry, *res;
  size_t qry_len;
  str_builder *sb;
  int retval = 0;

  if ((sb = strbld_create()) == 0) {
    return -1;
  }

  strbld_str(sb, "SHOW TABLES LIKE '", 0);
  modbTableName_sb(sb, modb, suffix, suffix_len);
  strbld_char(sb, '\'');

  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return -1;
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

uint64_t destroyMODBTable(stored_conn *sconn, modb_ref *modb, const char *suffix, size_t suffix_len)
{
  char *table;
  size_t table_len;
  char *qry;
  uint64_t res;
  size_t qry_len;
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return (uint64_t)-1;
  }
  modbTableName(&table, &table_len, modb, suffix, suffix_len);

  strbld_str(sb, "DROP TABLE IF EXISTS ", 0);
  escapeTableName_sb(sb, table, table_len);

  modbFreeTableName(&table);
  if (strbld_finalize_or_destroy(&sb, &qry, &qry_len) != 0) {
    return (uint64_t)-1;
  }

  res = simpleQuery(sconn, qry, qry_len);
  free(qry);

  return res;
}



uint64_t createSysTable(stored_conn *sconn, modb_ref *modb)
{
  return createMODBTable(
        sconn, modb,
        SYS_TABLE, STR_LEN(SYS_TABLE),
        "("
        "`key` VARCHAR(255) NULL, "
        "`value` VARCHAR(255) NULL, "
        "UNIQUE(`key`)"
        ")", 0
        );
}
uint64_t createMetaTable(stored_conn *sconn, modb_ref *modb)
{
  return createMODBTable(
        sconn, modb,
        METADATA_TABLE, STR_LEN(METADATA_TABLE),
        "("
        "`mdo_id` INT UNSIGNED NOT NULL AUTO_INCREMENT, "
        "`type` VARCHAR(255) NOT NULL, "
        "`title` VARCHAR(255) NOT NULL, "
        "`owner_id` INT UNSIGNED NOT NULL, "
        "`created` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, "
        "`updated` TIMESTAMP on update CURRENT_TIMESTAMP NULL DEFAULT NULL, "
        "`deleted` TIMESTAMP NULL DEFAULT NULL, "
        "PRIMARY KEY(`mdo_id`)"
        ")", 0
        );
}
uint64_t createObjectsTable(stored_conn *sconn, modb_ref *modb)
{
  return createMODBTable(
        sconn, modb,
        OBJECTS_TABLE, STR_LEN(OBJECTS_TABLE),
        "("
        "`mdo_id` INT UNSIGNED NOT NULL, "
        "`object` MEDIUMBLOB NULL, "
        "PRIMARY KEY (`mdo_id`)"
        ")", 0
        );
}
uint64_t createMDOGroupsTable(stored_conn *sconn, modb_ref *modb)
{
  return createMODBTable(
        sconn, modb,
        MDO_GROUPS_TABLE, STR_LEN(MDO_GROUPS_TABLE),
        "("
        "`mdo_id` INT UNSIGNED NOT NULL, "
        "`group_id` INT UNSIGNED NOT NULL, "
        "INDEX(`mdo_id`), "
        "INDEX(`group_id`)"
        ")", 0
        );
}

uint64_t createUsersTable(stored_conn *sconn, modb_ref *modb)
{
  return createMODBTable(
        sconn, modb,
        USERS_TABLE, STR_LEN(USERS_TABLE),
        "("
        "`id` INT UNSIGNED NOT NULL AUTO_INCREMENT, "
        "`username` VARCHAR(255) NOT NULL, "
        "`email` VARCHAR(255) NOT NULL, "
        "`created` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, "
        "`updated` TIMESTAMP on update CURRENT_TIMESTAMP NULL DEFAULT NULL, "
        "`deleted` TIMESTAMP NULL DEFAULT NULL, "
        "PRIMARY KEY (`id`), "
        "UNIQUE(`username`), "
        "UNIQUE(`email`)"
        ")", 0
        );
}
uint64_t createGroupsTable(stored_conn *sconn, modb_ref *modb)
{
  return createMODBTable(
        sconn, modb,
        GROUPS_TABLE, STR_LEN(GROUPS_TABLE),
        "("
        "`id` INT UNSIGNED NOT NULL AUTO_INCREMENT, "
        "`name` VARCHAR(255) NOT NULL, "
        "`created` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, "
        "`updated` TIMESTAMP on update CURRENT_TIMESTAMP NULL DEFAULT NULL, "
        "`deleted` TIMESTAMP NULL DEFAULT NULL, "
        "PRIMARY KEY (`id`), "
        "UNIQUE(`name`)"
        ")", 0
        );
}
uint64_t createUserGroupsTable(stored_conn *sconn, modb_ref *modb)
{
  return createMODBTable(
        sconn, modb,
        USER_GROUPS_TABLE, STR_LEN(USER_GROUPS_TABLE),
        "("
        "`user_id` INT UNSIGNED NOT NULL, "
        "`group_id` INT UNSIGNED NOT NULL, "
        "INDEX(`user_id`), "
        "INDEX(`group_id`)"
        ")", 0
        );
}

uint64_t createMetaExtTable(stored_conn *sconn, modb_ref *modb,
                            column_data **col_data, size_t cols)
{
  char *table_def;
  size_t table_def_len;
  uint64_t ret_val;

  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return (uint64_t)-1;
  }
  strbld_str(sb, "("
                 "`mdo_id` INT UNSIGNED NOT NULL", 0);
  for (size_t c = 0; c < cols; c++) {
    strbld_str(sb, ", ", 2);
    columnCreateStr_sb(sb, *(col_data + c));
  }
  strbld_str(sb, ", INDEX (`mdo_id`)"
                 ")", 0);
  if (strbld_finalize_or_destroy(&sb, &table_def, &table_def_len) != 0) {
    return (uint64_t)-1;
  }

  ret_val = createMODBTable(
        sconn, modb,
        META_EXT_TABLE, STR_LEN(META_EXT_TABLE),
        table_def, table_def_len
        );
  free(table_def);

  return ret_val;
}
