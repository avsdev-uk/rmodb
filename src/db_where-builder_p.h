#ifndef H__DB_WHERE_BUILDER_P__
#define H__DB_WHERE_BUILDER_P__

#include "db_where-builder.h"

struct where_logic_t {
  where_builder *up;
  e_where_logic logic_type;

  where_builder **clauses;
  size_t n_clauses;
  size_t n_alloc;
};
typedef struct where_logic_t where_logic;

struct where_clause_t {
  where_builder *up;
  e_where_logic logic_type;

  // Table
  char *table;
  size_t table_len;

  // Column
  char *col;
  size_t col_len;

  // Op
  e_where_op op;

  // Values
  char **values;
  size_t *values_len;
  size_t n_values;
  size_t n_alloc;
};
typedef struct where_clause_t where_clause;


where_logic *createLogic(e_where_logic type, size_t initial_size);
int compileLogic(where_logic *logic, char **str, size_t *str_len);
void freeLogic(where_logic **logic_ptr);

where_builder *appendLogicClause(where_builder *wb, where_builder *wb_clause);


where_clause *createWhere(const char *tbl, const char *col, e_where_op op);
int compileWhere(where_clause *clause, char **str, size_t *str_len);
void freeWhere(where_clause **where_ptr);

int ensureWhereValueSize(where_clause *clause, size_t new_size);


where_builder *where_And_Or(where_builder *wb, where_builder *wb_clause, e_where_logic and_or);
where_builder *where_In_notIn(where_builder *wb, const char *tbl, const char *col, e_where_op op,
                              e_column_type type, uint32_t n_args, ...);
where_builder *where_In_notIn_va(where_builder *wb, const char *tbl, const char *col, e_where_op op,
                                 e_column_type type, uint32_t n_args, va_list args);

#endif // H__DB_WHERE_BUILDER_P__
