#ifndef H__DB_WHERE_BUILDER__
#define H__DB_WHERE_BUILDER__

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#include "db_column.h"
#include "strext.h"


enum e_where_logic_t {
  UNK,
  CLAUSE,
  AND,
  OR
};

enum e_where_op_t {
  EQ,
  NEQ,
  GT,
  GTE,
  LT,
  LTE,
  IS_NULL,
  NOT_NULL,
  IN,
  NOT_IN
};

struct where_builder_t {
  struct where_builder_t *up;
  enum e_where_logic_t logic_type;
};

typedef enum e_where_logic_t e_where_logic;
typedef enum e_where_op_t e_where_op;
typedef struct where_builder_t where_builder;


where_builder *createWhereBuilder(where_builder *initial_clause);
int compileWhereBuilder(where_builder *wb, char **str, size_t *str_len, int free_wb);
void compileWhereBuilder_sb(where_builder *wb, str_builder *sb, int free_wb);
void freeWhereBuilder(where_builder **wb_ptr);


where_builder *where(const char *tbl, const char *col, e_where_op op, e_column_type type,
                     uint32_t n_args, ...);
where_builder *where_va(const char *tbl, const char *col, e_where_op op, e_column_type type,
                        uint32_t  n_args, va_list args);


where_builder *clearWhereValue(where_builder *wb_clause);
where_builder *setWhereValue(where_builder *wb_clause, e_column_type type, uint32_t n_args, ...);
where_builder *setWhereValue_va(where_builder *wb_clause,
                                e_column_type type, uint32_t n_args, va_list args);


where_builder *whereAnd(where_builder *wb, where_builder *wb_clause);
where_builder *whereOr(where_builder *wb, where_builder *wb_clause);

where_builder *whereIn(where_builder *wb, const char *tbl, const char *col,
                       e_column_type type, uint32_t n_args, ...);
where_builder *whereIn_va(where_builder *wb, const char *tbl, const char *col,
                          e_column_type type, uint32_t n_args, va_list args);
where_builder *whereNotIn(where_builder *wb, const char *tbl, const char *col,
                          e_column_type type, uint32_t n_args, ...);
where_builder *whereNotIn_va(where_builder *wb, const char *tbl, const char *col,
                             e_column_type type, uint32_t n_args, va_list args);

where_builder *nextWhere(where_builder *wb);
where_builder *finalizeWhere(where_builder *wb);



#endif // H__DB_WHERE_BUILDER__
