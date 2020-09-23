#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "db_where-builder.h"
#include "db_connection.h"
#include "db_value.h"
#include "strext.h"


struct where_logic_t {
  where_builder *up;
  e_where_logic logic_type;

  where_builder **constructs;
  size_t n_constructs;
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


// private:
void freeLogic(where_logic **logic_ptr)
{
  where_logic *logic;
  where_builder *construct;

  logic = *logic_ptr;
  while (logic->n_constructs > 0) {
    construct = logic->constructs[logic->n_constructs - 1];
    construct->up = 0;
    destroyWhereBuilder(&construct);
    logic->n_constructs--;
  }
  if (logic->constructs != 0) {
    free(logic->constructs);
    logic->constructs = 0;
  }
  free(*logic_ptr);
  *logic_ptr = 0;
}
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

  logic->constructs = (where_builder **)malloc(sizeof(where_builder *) * initial_size);
  if (logic->constructs == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    freeLogic(&logic);
    return 0;
  }
  memset(logic->constructs, 0, sizeof(where_builder *) * initial_size);
  logic->n_constructs = 0;
  logic->n_alloc = initial_size;

  return logic;
}
where_builder *appendClause(where_builder *wb, where_builder *wb_clause)
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

  if (logic->n_constructs == logic->n_alloc) {
    old_ptr = (where_builder **)logic->constructs;
    new_size = sizeof(where_builder *) * (logic->n_alloc + 1);
    logic->constructs = (where_builder **)realloc(logic->constructs, new_size);
    if (logic->constructs == 0) {
      fprintf(stderr, "[%d]realloc: (%d) %s\n", __LINE__, errno, strerror(errno));
      logic->constructs = old_ptr;
      return wb;
    }
    logic->n_alloc++;
  }

  logic->constructs[logic->n_constructs] = wb_clause;
  logic->n_constructs++;
  wb_clause->up = (where_builder *)logic;

  return wb;
}

int ensureValueSize(where_clause *clause, size_t new_size)
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
      freeWhere((where_builder **)&clause);
      return 0;
    }
  }
  if (strmemcpy(col, strlen(col), &clause->col, &clause->col_len) != 0) {
    freeWhere((where_builder **)&clause);
    return 0;
  }

  clause->values = (char **)malloc(sizeof(char *));
  clause->values_len = (size_t *)malloc(sizeof(size_t));
  if (clause->values == 0 || clause->values_len == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    freeWhere((where_builder **)&clause);
    return 0;
  }
  clause->n_alloc = 1;

  return clause;
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
        destroyWhereBuilder(&wb_clause);
        return wb;
      }
      free(wb);
      if (wb_clause != 0) {
        logic->constructs[0] = wb_clause;
        logic->n_constructs++;
        wb_clause->up = (where_builder *)logic;
      }
      wb = (where_builder *)logic;
      break;
    }

    case CLAUSE:
    {
      if ((logic = createLogic(and_or, 2)) == 0) {
        destroyWhereBuilder(&wb_clause);
        return wb;
      }
      logic->constructs[0] = wb;
      logic->n_constructs++;
      wb->up = (where_builder *)logic;
      if (wb_clause != 0) {
        logic->constructs[1] = wb_clause;
        logic->n_constructs++;
        wb_clause->up = (where_builder *)logic;
      }
      wb = (where_builder *)logic;
      break;
    }

    case OR:
    {
      if (and_or == AND) {
        if ((logic = createLogic(AND, 2)) == 0) {
          destroyWhereBuilder(&wb_clause);
          return wb;
        }
        if (whereOr(wb, (where_builder *)logic) == 0) {
          freeLogic(&logic);
          destroyWhereBuilder(&wb_clause);
          return wb;
        }
        wb = appendClause((where_builder *)logic, wb_clause);
      } else {
        wb = appendClause(wb, wb_clause);
      }
      break;
    }

    case AND:
    {
      if (and_or == AND) {
        wb = appendClause(wb, wb_clause);
      } else {
        if ((logic = createLogic(OR, 2)) == 0) {
          destroyWhereBuilder(&wb_clause);
          return wb;
        }
        if (whereAnd(wb, (where_builder *)logic) == 0) {
          freeLogic(&logic);
          destroyWhereBuilder(&wb_clause);
          return wb;
        }
        wb = appendClause((where_builder *)logic, wb_clause);
      }
      break;
    }
  }

  return wb;
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


// public:
where_builder *createWhereBuilder(where_builder *initial_clause)
{
  if (initial_clause != 0) {
    return initial_clause;
  }

  where_builder *wb = (where_builder *)malloc(sizeof(where_builder));
  if (wb == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }
  memset(wb, 0, sizeof(where_builder));

  return wb;
}
void destroyWhereBuilder(where_builder **wb_ptr)
{
  where_builder *wb = finalizeWhere(*wb_ptr);

  if (*wb_ptr == 0) {
    return;
  }

  switch(wb->logic_type) {
    case CLAUSE:
    {
      freeWhere(&wb);
      break;
    }
    case OR:
    case AND:
    {
      freeLogic((where_logic **)&wb);
      break;
    }
    case UNK:
    {
      free(*wb_ptr);
      break;
    }
  }

  *wb_ptr = 0;
}


where_builder *where(const char *tbl, const char *col, e_where_op op, e_column_type type,
                     uint32_t n_args, ...)
{
  va_list args;
  where_builder *ret;

  va_start(args, n_args);
  ret = where_va(tbl, col, op, type, n_args, args);
  va_end(args);

  return ret;
}
where_builder *where_va(const char *tbl, const char *col, e_where_op op, e_column_type type,
                        uint32_t n_args, va_list args)
{
  where_builder *wb_clause = (where_builder *)createWhere(tbl, col, op);

  if (wb_clause == 0) {
    return 0;
  }

  return setWhereValue_va(wb_clause, type, n_args, args);
}
void freeWhere(where_builder **wb_clause_ptr)
{
  where_clause *clause = (where_clause *)(*wb_clause_ptr);

  if (wb_clause_ptr == 0 || (*wb_clause_ptr)->logic_type != CLAUSE) {
    fprintf(stderr, "[%d]freeWhere: Where-builder stack corrupted!\n", __LINE__);
    return;
  }

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

  if (clause->table) {
    free(clause->table);
    clause->table = 0;
  }

  free(clause);
  *wb_clause_ptr = 0;
}


where_builder *clearWhereValue(where_builder *wb_clause)
{
  where_clause *clause = (where_clause *)wb_clause;

  if (wb_clause->logic_type != CLAUSE) {
    fprintf(stderr, "[%d]clearWhereValue: Where-builder stack corrupted!\n", __LINE__);
    return wb_clause;
  }

  while (clause->n_values > 0) {
    clause->n_values--;
    free(clause->values[clause->n_values]);
    clause->values[clause->n_values] = 0;
    clause->values_len[clause->n_values] = 0;
  }

  return wb_clause;
}
where_builder *setWhereValue(where_builder *wb_clause, e_column_type type, uint32_t n_args, ...)
{
  va_list args;
  where_builder *ret;

  va_start(args, n_args);
  ret = setWhereValue_va(wb_clause, type, n_args, args);
  va_end(args);

  return ret;
}
where_builder *setWhereValue_va(where_builder *wb_clause,
                                e_column_type type, uint32_t n_args, va_list args)
{
  where_clause *clause = (where_clause *)wb_clause;
  char *value;
  size_t value_len;

  if (wb_clause->logic_type != CLAUSE) {
    fprintf(stderr, "[%d]setWhereValue: Where-builder stack corrupted!\n", __LINE__);
    return wb_clause;
  }

  switch (clause->op) {
    case EQ:
    case NEQ:
    case GT:
    case GTE:
    case LT:
    case LTE:
    {
      clearWhereValue(wb_clause);

      db_value_va(&value, &value_len, type, n_args, args);

      if (value != 0) {
        clause->values[clause->n_values] = value;
        clause->values_len[clause->n_values] = value_len;
      }
      clause->n_values++;
      break;
    }

    case IS_NULL:
    case NOT_NULL:
    {
      break;
    }

    case IN:
    case NOT_IN:
    {
      if (ensureValueSize(clause, clause->n_values + n_args) != 0) {
        freeWhere(&wb_clause);
        return 0;
      }

      for (uint32_t i = 0; i < n_args; i++) {
        db_value_va(&value, &value_len, type, 1, args);
        if (value != 0) {
          clause->values[clause->n_values] = value;
          clause->values_len[clause->n_values] = value_len;
        }
        clause->n_values++;
      }

      break;
    }
  }

  return wb_clause;
}


where_builder *whereAnd(where_builder *wb, where_builder *wb_clause)
{
  return where_And_Or(wb, wb_clause, AND);
}
where_builder *whereOr(where_builder *wb, where_builder *wb_clause)
{
  return where_And_Or(wb, wb_clause, OR);
}

where_builder *whereIn(where_builder *wb, const char *tbl, const char *col,
                       e_column_type type, uint32_t n_args, ...)
{
   where_builder *ret;
   va_list args;

   va_start(args, n_args);
   ret = whereIn_va(wb, tbl, col, type, n_args, args);
   va_end(args);

   return ret;
}
where_builder *whereIn_va(where_builder *wb, const char *tbl, const char *col,
                          e_column_type type, uint32_t n_args, va_list args)
{
  return where_In_notIn_va(wb, tbl, col, IN, type, n_args, args);
}
where_builder *whereNotIn(where_builder *wb, const char *tbl, const char *col,
                          e_column_type type, uint32_t n_args, ...)
{
  where_builder *ret;
  va_list args;

  va_start(args, n_args);
  ret = whereIn_va(wb, tbl, col, type, n_args, args);
  va_end(args);

  return ret;
}
where_builder *whereNotIn_va(where_builder *wb, const char *tbl, const char *col,
                             e_column_type type, uint32_t n_args, va_list args)
{

  return where_In_notIn_va(wb, tbl, col, NOT_IN, type, n_args, args);
}

where_builder *whereNext(where_builder *wb)
{
  if (wb->up == 0) {
    fprintf(stderr, "[%d]whereEnd: Where-builder stack corrupted!\n", __LINE__);
    return wb;
  }

  return wb->up;
}
where_builder *finalizeWhere(where_builder *wb)
{
  while (wb->up != 0) {
    wb = wb->up;
  }
  return wb;
}


int compileWhere(where_builder *wb, char **str, size_t *str_len)
{
  where_logic *logic;
  where_clause *clause;
  struct str_builder_t *sb;
  char *tmp;
  size_t tmp_len;
  size_t idx;

  sb = strbld_create();
  if (sb == 0) {
    return -1;
  }

  switch(wb->logic_type) {
    case UNK:
      strbld_destroy(&sb);
      return -1;
    case CLAUSE:
      {
        clause = (where_clause *)wb;

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
      break;
    case OR:
    case AND:
    {
      logic = (where_logic *)wb;

      if (logic->n_constructs == 0) {
        break;
      }

      strbld_char(sb, '(');
      for (size_t i = 0; i < logic->n_constructs; i++) {
        if (compileWhere(logic->constructs[i], &tmp, &tmp_len) == 0) {
          strbld_str(sb, tmp, tmp_len);
          free(tmp);
        }
        if (i < (logic->n_constructs - 1)) {
          strbld_str(sb, (wb->logic_type == OR ? " OR " : " AND "), (wb->logic_type == OR ? 4 : 5));
        }
      }
      strbld_char(sb, ')');
      break;
    }
  }

  return strbld_finalize_or_destroy(&sb, str, str_len);
}
