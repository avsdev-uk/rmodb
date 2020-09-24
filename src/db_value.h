#ifndef H__DB_VALUE__
#define H__DB_VALUE__

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#include "db_column.h"

char *db_value(char **str, size_t *len, e_column_type type, uint32_t n_args, ...);
char *db_value_va(char **str, size_t *len, e_column_type type, uint32_t n_args, va_list args);

#endif // H__DB_VALUE__
