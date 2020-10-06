#ifndef H__DB_VALUE__
#define H__DB_VALUE__

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#include "db_column.h"
#include "strext.h"

char *db_value(char **str, size_t *len, e_column_type type, uint32_t n_args, ...);
char *db_value_va(char **str, size_t *len, e_column_type type, uint32_t n_args, va_list args);
void db_value_sb(str_builder *sb, e_column_type type, uint32_t n_args, ...);
void db_value_sbva(str_builder *sb, e_column_type type, uint32_t n_args, va_list args);

int64_t db_timestampUnix(const char *ts);
char *db_timestampString(int64_t ts_unix, char **str, size_t *len);

#endif // H__DB_VALUE__
