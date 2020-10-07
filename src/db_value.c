#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <errno.h>
#include <mysql.h>
#include <time.h>

#include "db_value.h"
#include "strext.h"


char *db_value(char **str, size_t *len, e_column_type type, uint32_t n_args, ...)
{
  va_list args;
  char *ret;

  va_start(args, n_args);
  ret = db_value_va(str, len, type, n_args, args);
  va_end(args);

  return ret;
}
char *db_value_va(char **str, size_t *len, e_column_type type, uint32_t n_args, va_list args)
{
  str_builder *sb;

  if ((sb = strbld_create()) == 0) {
    return 0;
  }
  db_value_sbva(sb, type, n_args, args);
  if (strbld_finalize_or_destroy(&sb, str, len) != 0) {
    return 0;
  }

  return *str;
}

void db_value_sb(str_builder *sb, e_column_type type, uint32_t n_args, ...)
{
  va_list args;
  va_start(args, n_args);
  db_value_sbva(sb, type, n_args, args);
  va_end(args);
}
void db_value_sbva(str_builder *sb, e_column_type type, uint32_t n_args, va_list args)
{
  char buf[32], *tmp_str = 0, *esc_str = 0;
  size_t tmp_len = 0, esc_len = 0;
  MYSQL *sql;
  int nchar;

  if (n_args == 0) {
    return;
  }

  if (type == TYPE_RAW) {
    tmp_str = va_arg(args, char *);
    tmp_len = va_arg(args, size_t);
    strbld_str(sb, tmp_str, tmp_len);
    return;
  }

  if (type == TYPE_TIMESTAMP) {
    if (db_timestampString(va_arg(args, unsigned int), &tmp_str, &tmp_len) == 0) {
      strbld_str(sb, tmp_str, tmp_len);
      free(tmp_str);
    }
    return;
  }

  if (type == TYPE_STRING) {
    tmp_str = va_arg(args, char *);
    if (tmp_str == 0) {
      strbld_str(sb, "NULL", 4);
      return;
    }

    if (n_args == 1) {
      tmp_len = strlen(tmp_str);
    } else {
      tmp_len = va_arg(args, size_t);
    }


    esc_str = (char *)malloc(tmp_len * 2 + 1 + 2);
    if (esc_str == 0) {
      fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
      return;
    }

    sql = mysql_init(NULL);
    if (sql == 0) {
      fprintf(stderr, "[%d]mysql_init: (%d) %s\n", __LINE__, errno, strerror(errno));
      free(esc_str);
      return;
    }
    esc_str[0] = '\'';
    esc_len = mysql_real_escape_string(sql, esc_str + 1, tmp_str, tmp_len);
    esc_str[esc_len + 1] = '\'';
    esc_str[esc_len + 2] = '\0';
    esc_len += 2;
    mysql_close(sql);

    strbld_str(sb, esc_str, esc_len);

    free(esc_str);
    return;
  }

  if (type == TYPE_BLOB) {
    tmp_str = (char *)va_arg(args, void *);
    if (tmp_str == 0) {
      strbld_str(sb, "NULL", 4);
      return;
    }
    tmp_len = va_arg(args, size_t);

    esc_str = (char *)malloc(tmp_len * 2 + 1 + 2);
    if (esc_str == 0) {
      fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
      return;
    }
    esc_str[0] = '0';
    esc_str[1] = 'x';
    esc_len = mysql_hex_string(esc_str + 2, tmp_str, tmp_len);

    strbld_str(sb, esc_str, esc_len);

    free(esc_str);
    return;
  }

  memset(buf, 0, 32);
  switch(type) {
    case TYPE_BOOL:
    {
      nchar = sprintf(buf, "%s", va_arg(args, int) == 0 ? "FALSE" : "TRUE");
      break;
    }

    case TYPE_INT8:
    {
      nchar = vsprintf(buf, "%"PRIi8, args);
      break;
    }
    case TYPE_UINT8:
    {
      nchar = vsprintf(buf, "%"PRIu8, args);
      break;
    }

    case TYPE_INT16:
    {
      nchar = vsprintf(buf, "%"PRIi16, args);
      break;
    }
    case TYPE_UINT16:
    {
      nchar = vsprintf(buf, "%"PRIu16, args);
      break;
    }

    case TYPE_INT32:
    {
      nchar = vsprintf(buf, "%"PRIi32, args);
      break;
    }
    case TYPE_UINT32:
    {
      nchar = vsprintf(buf, "%"PRIu32, args);
      break;
    }

    case TYPE_INT64:
    {
      nchar = vsprintf(buf, "%"PRIi64, args);
      break;
    }
    case TYPE_UINT64:
    {
      nchar = vsprintf(buf, "%"PRIu64, args);
      break;
    }

    case TYPE_FLOAT:
    {
      nchar = vsprintf(buf, "%f", args);
      break;
    }
    case TYPE_DOUBLE:
    {
      nchar = vsprintf(buf, "%lf", args);
      break;
    }

    case TYPE_ID:
    {
      nchar = vsprintf(buf, "%"PRIu32, args);
      break;
    }

    default:
    {
      return;
    }
  }
  strbld_str(sb, buf, (size_t)nchar);
}


int64_t db_timestampUnix(const char *ts_str)
{
  struct tm tmval;
  time_t tunix;

  memset(&tmval, 0, sizeof(struct tm));
  if (sscanf(ts_str, "%d-%d-%d %d:%d:%d",
         &tmval.tm_year, &tmval.tm_mon, &tmval.tm_mday,
         &tmval.tm_hour, &tmval.tm_min, &tmval.tm_sec) != 6) {
    return 0;
  }

  tmval.tm_mon -= 1;
  tmval.tm_year -= 1900;
  tmval.tm_isdst = -1;

  tunix = mktime(&tmval);

  return tunix;
}
char *db_timestampString(int64_t ts_unix, char **str, size_t *str_len)
{
  struct tm *tmval;

  *str = (char *)malloc(20);
  if (*str == 0) {
    return 0;
  }
  memset(*str, 0, 20);

  tmval = gmtime((time_t *)&ts_unix);
  *str_len = strftime(*str, 20, "%Y-%m-%d %H:%M:%S", tmval);

  return *str;
}
