#include "stdio.h"
#include "errno.h"
#include "string.h"

#include "db_where-builder_p.h"


where_logic *createLogic(e_where_logic type, size_t initial_size)
{
  where_logic *logic = (where_logic *)malloc(sizeof(where_logic));
  if (logic == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }
  memset(logic, 0, sizeof(where_logic));

  logic->logic_type = type;

  if (initial_size == 0) {
    return logic;
  }

  logic->clauses = (where_builder **)malloc(sizeof(where_builder *) * initial_size);
  if (logic->clauses == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    freeLogic(&logic);
    return 0;
  }
  memset(logic->clauses, 0, sizeof(where_builder *) * initial_size);
  logic->n_clauses = 0;
  logic->n_alloc = initial_size;

  return logic;
}
int compileLogic(where_logic *logic, char **str, size_t *str_len)
{
  struct str_builder_t *sb;

  if ((sb = strbld_create()) == 0) {
    return -1;
  }

  compileLogic_sb(logic, sb);

  return strbld_finalize_or_destroy(&sb, str, str_len);
}
void compileLogic_sb(where_logic *logic, str_builder *sb)
{
  if (logic->n_clauses == 0) {
    return;
  }

  strbld_char(sb, '(');
  for (size_t i = 0; i < logic->n_clauses; i++) {
    compileWhereBuilder_sb(logic->clauses[i], sb, 0);
    if (i < (logic->n_clauses - 1)) {
      if (logic->logic_type == OR) {
        strbld_str(sb, " OR ", 4);
      } else {
        strbld_str(sb, " AND ", 5);
      }
    }
  }
  strbld_char(sb, ')');
}
void freeLogic(where_logic **logic_ptr)
{
  where_logic *logic;
  where_builder *construct;

  logic = *logic_ptr;
  while (logic->n_clauses > 0) {
    construct = logic->clauses[logic->n_clauses - 1];
    construct->up = 0;
    freeWhereBuilder(&construct);
    logic->n_clauses--;
  }
  if (logic->clauses != 0) {
    free(logic->clauses);
    logic->clauses = 0;
  }
  free(*logic_ptr);
  *logic_ptr = 0;
}

where_builder *appendLogicClause(where_builder *wb, where_builder *wb_clause)
{
  where_logic *logic = (where_logic *)wb;
  where_builder **old_ptr;
  size_t new_size;

  if (wb_clause == 0) {
    return wb;
  }

  if (!(wb->logic_type == AND || wb->logic_type == OR)) {
    fprintf(stderr, "[%d]appendClause: Where-builder stack corrupted!\n", __LINE__);
    return wb;
  }

  if (logic->n_clauses == logic->n_alloc) {
    old_ptr = (where_builder **)logic->clauses;
    new_size = sizeof(where_builder *) * (logic->n_alloc + 1);
    logic->clauses = (where_builder **)realloc(logic->clauses, new_size);
    if (logic->clauses == 0) {
      fprintf(stderr, "[%d]realloc: (%d) %s\n", __LINE__, errno, strerror(errno));
      logic->clauses = old_ptr;
      return wb;
    }
    logic->n_alloc++;
  }

  logic->clauses[logic->n_clauses] = wb_clause;
  logic->n_clauses++;
  wb_clause->up = (where_builder *)logic;

  return wb;
}


where_clause *createWhere(const char *tbl, const char *col, e_where_op op)
{
  where_clause *clause;

  clause = (where_clause *)malloc(sizeof(where_clause));
  if (clause == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }
  memset(clause, 0, sizeof(where_clause));

  clause->logic_type = CLAUSE;
  clause->op = op;

  if (tbl != 0) {
    if (strmemcpy(tbl, strlen(tbl), &clause->table, &clause->table_len) != 0) {
      freeWhere(&clause);
      return 0;
    }
  }
  if (strmemcpy(col, strlen(col), &clause->col, &clause->col_len) != 0) {
    freeWhere(&clause);
    return 0;
  }

  clause->values = (char **)malloc(sizeof(char *));
  clause->values_len = (size_t *)malloc(sizeof(size_t));
  if (clause->values == 0 || clause->values_len == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    freeWhere(&clause);
    return 0;
  }
  clause->n_alloc = 1;

  return clause;
}
int compileWhere(where_clause *clause, char **str, size_t *str_len)
{
  struct str_builder_t *sb;

  if ((sb = strbld_create()) == 0) {
    return -1;
  }

  compileWhere_sb(clause, sb);

  return strbld_finalize_or_destroy(&sb, str, str_len);
}
void compileWhere_sb(where_clause *clause, str_builder *sb)
{
  size_t idx;

  // Column
  if (clause->table != 0) {
    strbld_char(sb, '`');
    strbld_str(sb, clause->table, clause->table_len);
    strbld_char(sb, '`');
    strbld_char(sb, '.');
  }
  strbld_char(sb, '`');
  strbld_str(sb, clause->col, clause->col_len);
  strbld_char(sb, '`');
  strbld_char(sb, ' ');

  // Op
  switch(clause->op) {
    case EQ:
      strbld_char(sb, '=');
      break;
    case NEQ:
      strbld_char(sb, '!');
      strbld_char(sb, '=');
      break;
    case GT:
      strbld_char(sb, '>');
      break;
    case GTE:
      strbld_char(sb, '>');
      strbld_char(sb, '=');
      break;
    case LT:
      strbld_char(sb, '<');
      break;
    case LTE:
      strbld_char(sb, '<');
      strbld_char(sb, '=');
      break;
    case IS_NULL:
      strbld_str(sb, "IS NULL", 7);
      break;
    case NOT_NULL:
      strbld_str(sb, "IS NOT NULL", 11);
      break;
    case IN:
      strbld_str(sb, "IN(", 3);
      break;
    case NOT_IN:
      strbld_str(sb, "NOT IN(", 7);
      break;
  }

  // Value
  for (idx = 0; idx < clause->n_values; idx++) {
    strbld_char(sb, ' ');
    strbld_str(sb, clause->values[idx], clause->values_len[idx]);
    if (idx < clause->n_values - 1 && (clause->op == IN || clause->op == NOT_IN)) {
      strbld_char(sb, ',');
    }
  }

  if (clause->op == IN || clause->op == NOT_IN) {
    strbld_char(sb, ' ');
    strbld_char(sb, ')');
  }
}
void freeWhere(where_clause **where_ptr)
{
  where_clause *clause = *where_ptr;

  while (clause->n_values > 0) {
    free(clause->values[clause->n_values - 1]);
    clause->n_values--;
  }
  if (clause->values != 0) {
    free(clause->values);
    clause->values = 0;
  }
  if (clause->values_len != 0) {
    free(clause->values_len);
    clause->values_len = 0;
  }

  if (clause->col != 0) {
    free(clause->col);
    clause->col = 0;
  }

  if (clause->table != 0) {
    free(clause->table);
    clause->table = 0;
  }

  free(clause);
  *where_ptr = 0;
}

int ensureWhereValueSize(where_clause *clause, size_t new_size)
{
  size_t *old_lens;
  char **old_values;

  if (clause->n_alloc >= new_size) {
    return 0;
  }

  old_values = clause->values;
  clause->values = (char **)realloc(clause->values, sizeof(char *) * new_size);
  if (clause->values == 0) {
    fprintf(stderr, "[%d]realloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    clause->values = old_values;
    return 1;
  }

  old_lens = clause->values_len;
  clause->values_len = (size_t *)realloc(clause->values_len, sizeof(size_t) * new_size);
  if (clause->values_len == 0) {
    fprintf(stderr, "[%d]realloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    clause->values_len = old_lens;
    return 1;
  }

  (clause->n_alloc)++;

  return 0;
}


where_builder *where_And_Or(where_builder *wb, where_builder *wb_clause, e_where_logic and_or)
{
  where_logic *logic;

  if (wb == 0) {
    wb = (where_builder *)createLogic(and_or, 2);
    if (wb == 0) {
      return wb_clause;
    }
  }

  switch(wb->logic_type) {
    case UNK:
    {
      if ((logic = createLogic(and_or, 2)) == 0) {
        freeWhereBuilder(&wb_clause);
        return wb;
      }
      free(wb);
      if (wb_clause != 0) {
        logic->clauses[0] = wb_clause;
        logic->n_clauses++;
        wb_clause->up = (where_builder *)logic;
      }
      wb = (where_builder *)logic;
      break;
    }

    case CLAUSE:
    {
      if ((logic = createLogic(and_or, 2)) == 0) {
        freeWhereBuilder(&wb_clause);
        return wb;
      }
      logic->clauses[0] = wb;
      logic->n_clauses++;
      wb->up = (where_builder *)logic;
      if (wb_clause != 0) {
        logic->clauses[1] = wb_clause;
        logic->n_clauses++;
        wb_clause->up = (where_builder *)logic;
      }
      wb = (where_builder *)logic;
      break;
    }

    case OR:
    {
      if (and_or == AND) {
        if ((logic = createLogic(AND, 2)) == 0) {
          freeWhereBuilder(&wb_clause);
          return wb;
        }
        if (whereOr(wb, (where_builder *)logic) == 0) {
          freeLogic(&logic);
          freeWhereBuilder(&wb_clause);
          return wb;
        }
        wb = appendLogicClause((where_builder *)logic, wb_clause);
      } else {
        wb = appendLogicClause(wb, wb_clause);
      }
      break;
    }

    case AND:
    {
      if (and_or == AND) {
        wb = appendLogicClause(wb, wb_clause);
      } else {
        if ((logic = createLogic(OR, 2)) == 0) {
          freeWhereBuilder(&wb_clause);
          return wb;
        }
        if (whereAnd(wb, (where_builder *)logic) == 0) {
          freeLogic(&logic);
          freeWhereBuilder(&wb_clause);
          return wb;
        }
        wb = appendLogicClause((where_builder *)logic, wb_clause);
      }
      break;
    }
  }

  return wb;
}
where_builder *where_In_notIn(where_builder *wb, const char *tbl, const char *col, e_where_op op,
                              e_column_type type, uint32_t n_args, ...)
{
  where_builder *ret;
  va_list args;

  va_start(args, n_args);
  ret = where_In_notIn_va(wb, tbl, col, op, type, n_args, args);
  va_end(args);

  return ret;
}
where_builder *where_In_notIn_va(where_builder *wb, const char *tbl, const char *col, e_where_op op,
                                 e_column_type type, uint32_t n_args, va_list args)
{
  where_clause *clause;
  where_builder *wb_clause;

  if (wb == 0) {
    return where_va(tbl, col, op, type, n_args, args);
  }

  switch (wb->logic_type) {
    case UNK:
    {
      free(wb);
      return where_va(tbl, col, op, type, n_args, args);
    }
    case CLAUSE:
    {
      clause = (where_clause *)wb;
      if (clause->op != op
          || (tbl != 0 && clause->table == 0)
          || (tbl != 0 && clause->table != 0 && strcmp(clause->table, tbl) != 0)
          || (col != 0 && clause->col == 0)
          || (col != 0 && clause->col != 0 && strcmp(clause->col, col) != 0)) {
        if (wb->up == 0) {
          fprintf(stderr, "[%d]whereIn_notIn: Where-builder stack corrupted!\n", __LINE__);
          return wb;
        }
        return where_In_notIn_va(wb->up, tbl, col, op, type, n_args, args);
      }

      return setWhereValue_va((where_builder *)clause, type, n_args, args);
    }
    case AND:
    case OR:
    {
      if ((wb_clause = where_va(tbl, col, op, type, n_args, args)) == 0) {
        return wb;
      }
      if (wb->logic_type == AND) {
        whereAnd(wb, wb_clause);
      } else {
        whereOr(wb, wb_clause);
      }
      return wb_clause;
    }
  }

  return wb;
}

