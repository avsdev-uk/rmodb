#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <errno.h>
#include <mysql.h>

#include "strext.h"
#include "db_value.h"

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
  char buf[32], *tmp_str = 0, *esc_str = 0;
  size_t tmp_len = 0, esc_len = 0;
  MYSQL *sql;

  if (n_args == 0) {
    return 0;
  }

  memset(buf, 0, 32);

  switch(type) {
    case TYPE_BOOL:
    {
      sprintf(buf, "%s", va_arg(args, int) == 0 ? "FALSE" : "TRUE");
      break;
    }

    case TYPE_INT8:
    {
      vsprintf(buf, "%"PRIi8, args);
      break;
    }
    case TYPE_UINT8:
    {
      vsprintf(buf, "%"PRIu8, args);
      break;
    }

    case TYPE_INT16:
    {
      vsprintf(buf, "%"PRIi16, args);
      break;
    }
    case TYPE_UINT16:
    {
      vsprintf(buf, "%"PRIu16, args);
      break;
    }

    case TYPE_INT32:
    {
      vsprintf(buf, "%"PRIi32, args);
      break;
    }
    case TYPE_TIMESTAMP:
    case TYPE_UINT32:
    {
      vsprintf(buf, "%"PRIu32, args);
      break;
    }

    case TYPE_INT64:
    {
      vsprintf(buf, "%"PRIi64, args);
      break;
    }
    case TYPE_UINT64:
    {
      vsprintf(buf, "%"PRIu64, args);
      break;
    }

    case TYPE_FLOAT:
    {
      vsprintf(buf, "%f", args);
      break;
    }
    case TYPE_DOUBLE:
    {
      vsprintf(buf, "%lf", args);
      break;
    }

    case TYPE_STRING:
    {
      tmp_str = va_arg(args, char *);
      if (n_args == 1) {
        tmp_len = strlen(tmp_str);
      } else {
        tmp_len = va_arg(args, size_t);
      }
      __attribute__((fallthrough));
    }
    case TYPE_BLOB:
    {
      if (tmp_str == 0) {
        tmp_str = (char *)va_arg(args, void *);
        tmp_len = va_arg(args, size_t);
      }

      if (tmp_str == 0) {
        if (strmemcpy("NULL", 4, &esc_str, &esc_len) != 0) {
          return 0;
        }
        break;
      }

      sql = mysql_init(NULL);
      if (sql == 0) {
        fprintf(stderr, "[%d]mysql_init: (%d) %s\n", __LINE__, errno, strerror(errno));
        return 0;
      }

      esc_str = (char *)malloc(tmp_len * 2 + 1 + 2);
      if (esc_str == 0) {
        fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
        mysql_close(sql);
        return 0;
      }


      esc_str[0] = '\'';
      esc_len = mysql_real_escape_string(sql, esc_str + 1, tmp_str, tmp_len);
      esc_str[esc_len + 1] = '\'';
      esc_str[esc_len + 2] = '\0';
      esc_len += 2;
      mysql_close(sql);

      tmp_str = esc_str;
      esc_str = (char *)realloc(esc_str, esc_len + 1);
      if (esc_str == 0) {
        fprintf(stderr, "[%d]realloc: (%d) %s\n", __LINE__, errno, strerror(errno));
        return 0;
      }

      break;
    }

    case TYPE_RAW:
    {
      tmp_str = va_arg(args, char *);
      tmp_len = va_arg(args, size_t);

      if (strmemcpy(tmp_str, tmp_len, &esc_str, &esc_len) != 0) {
        return 0;
      }
      break;
    }
  }

  if (esc_str == 0) {
    if (strmemcpy(buf, strlen(buf), &esc_str, &esc_len) != 0) {
      return 0;
    }
  }

  if (str != 0) {
    *str = esc_str;
    *len = esc_len;
  }

  return esc_str;
}
