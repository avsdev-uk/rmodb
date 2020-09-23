#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "strext.h"

size_t strlen_norm(const char *str, size_t str_len)
{
  if (str == 0) {
    return 0;
  }
  if (str_len == 0) {
    return strlen(str);
  }
  while (str[str_len - 1] == '\0') {
    str_len--;
  }
  return str_len;
}

int strmemcpy(const char *str, size_t str_len, char **new_str, size_t *new_len)
{
  if (str == 0) {
    return -1;
  }

  str_len = strlen_norm(str, str_len);

  *new_str = (char *)malloc(str_len + 1);
  if (*new_str == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return -errno;
  }

  memcpy(*new_str, str, str_len);
  (*new_str)[str_len] = '\0';
  (*new_len) = str_len;

  return 0;
}

size_t strapp(char **out, const char *a, size_t a_len)
{
  size_t s_len;
  if (a[a_len - 1] == '\0') {
    a_len -= 1;
  }

  if (*out == 0) {
    s_len = 0;
    *out = (char *)malloc(a_len + 1);
    if (*out == 0) {
      fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
      return 0;
    }
  } else {
    s_len = strlen(*out);
    *out = (char *)realloc(*out, s_len + a_len + 1);
    if (*out == 0) {
      fprintf(stderr, "[%d]realloc: (%d) %s\n", __LINE__, errno, strerror(errno));
      return 0;
    }
  }
  strncpy(*out + s_len, a, a_len);
  *(*out + s_len + a_len) = '\0';

  return s_len + a_len;
}

size_t strcmb(char **out, const char *a, size_t a_len, const char *b, size_t b_len)
{
  size_t s_len, t_len;
  if (a_len == 0) {
    a_len = strlen(a);
  } else if (a[a_len - 1] == '\0') {
    a_len -= 1;
  }
  if (b_len == 0) {
    b_len = strlen(b);
  } else if (b[b_len - 1] == '\0') {
    b_len -= 1;
  }
  t_len = a_len + b_len;

  if (*out == 0) {
    s_len = 0;
    *out = (char *)malloc(t_len + 1);
    if (*out == 0) {
      fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
      return 0;
    }
  } else {
    s_len = strlen(*out);
    *out = (char *)realloc(*out, s_len + t_len + 1);
    if (*out == 0) {
      fprintf(stderr, "[%d]realloc: (%d) %s\n", __LINE__, errno, strerror(errno));
      return 0;
    }
  }
  strncpy(*out + s_len, a, a_len);
  strncpy(*out + s_len + a_len, b, b_len);
  *(*out + s_len + t_len) = '\0';

  return s_len + t_len;
}


str_builder *strbld_create()
{
  str_builder *sb = malloc(sizeof(str_builder));
  if (sb == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }

  sb->str = (char *)malloc(32);
  if (sb == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    free(sb);
    return 0;
  }
  memset(sb->str, '\0', 32);

  sb->alloc = 32;
  sb->len = 0;
  sb->fails = 0;

  return sb;
}
void strbld_destroy(str_builder **sbp)
{
  str_builder *sb = *sbp;
  if (sb->alloc > 0 && sb->str != 0) {
    free(sb->str);
    sb->alloc = 0;
    sb->str = 0;
  }
  free(sb);
  *sbp = 0;
}
void strbld_finalize(str_builder **sbp, char **str, size_t *len)
{
  str_builder *sb = *sbp;
  *str = sb->str;
  *len = sb->len;
  sb->alloc = 0;
  strbld_destroy(sbp);
}
int strbld_finalize_or_destroy(str_builder **sbp, char **str, size_t *len)
{
  if ((*sbp)->fails > 0) {
    strbld_destroy(sbp);
    return -1;
  } else {
    strbld_finalize(sbp, str, len);
    return 0;
  }
}

int strbld_ensure_len(str_builder *sb, size_t len, int absolute)
{
  char *new_str;
  if (sb->alloc > len) {
    return 1;
  }

  if (absolute) {
    sb->alloc = len;
  } else {
    while (sb->alloc <= len) {
      sb->alloc <<= 1;
    }
  }

  new_str = (char *)realloc(sb->str, sb->alloc);
  if (new_str == 0) {
    fprintf(stderr, "[%d]realloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }
  sb->str = new_str;
  memset(sb->str + sb->len, '\0', sb->alloc - sb->len);

  return 1;
}

int strbld_str(str_builder *sb, const char *str, size_t len)
{
  if (sb->fails > 0) {
    return 0;
  }

  if (len == 0) {
    len = strlen(str);
  }
  if (str[len - 1] == '\0') {
    len--;
  }

  if (!strbld_ensure_len(sb, sb->len + len, 0)) {
    sb->fails++;
    return 0;
  }
  memmove(sb->str + sb->len, str, len);
  sb->len += len;

  return 1;
}
int strbld_char(str_builder *sb, const char c)
{
  if (sb->fails > 0) {
    return 0;
  }
  if (!strbld_ensure_len(sb, sb->len + 1, 0)) {
    sb->fails++;
    return 0;
  }
  *(sb->str + sb->len) = c;
  sb->len++;
  return 1;
}
