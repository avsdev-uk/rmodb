#ifndef H__STREXT__
#define H__STREXT__

#include <stddef.h>

#define STR_LEN(x) (sizeof(x)/sizeof(x[0])-1)

struct str_builder_t {
  char *str;
  size_t alloc;
  size_t len;
  size_t fails;
};
typedef struct str_builder_t str_builder;

size_t strlen_norm(const char *str, size_t str_len);
int strmemcpy(const char *str, size_t str_len, char **new_str, size_t *new_len);

size_t strapp(char **out, const char *a, size_t a_len);
size_t strcmb(char **out, const char *a, size_t a_len, const char *b, size_t b_len);

str_builder *strbld_create(void);
void strbld_destroy(str_builder **sb);
void strbld_finalize(str_builder **sb, char **str, size_t *len);
int strbld_finalize_or_destroy(str_builder **sb, char **str, size_t *len);

int strbld_ensure_len(str_builder *sb, size_t len, int absolute);

int strbld_seek(str_builder *sb, size_t to, int zero);
size_t strbld_len(str_builder *sb);

int strbld_str(str_builder *sb, const char *str, size_t len);
int strbld_char(str_builder *sb, const char c);

#endif // H__STREXT__
