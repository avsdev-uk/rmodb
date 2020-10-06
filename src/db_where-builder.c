#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "db_where-builder.h"
#include "db_where-builder_p.h"
#include "db_connection.h"
#include "db_value.h"
#include "strext.h"

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
int compileWhereBuilder(where_builder *wb, char **str, size_t *str_len)
{
  struct str_builder_t *sb;

  if ((sb = strbld_create()) == 0) {
    return -1;
  }

  compileWhereBuilder_sb(wb, sb);

  return strbld_finalize_or_destroy(&sb, str, str_len);
}
void compileWhereBuilder_sb(where_builder *wb, str_builder *sb)
{
  switch(wb->logic_type) {
    case CLAUSE:
    {
      compileWhere_sb((where_clause *)wb, sb);
      break;
    }
    case OR:
    case AND:
    {
      compileLogic_sb((where_logic *)wb, sb);
      break;
    }
    default:
    {
      break;
    }
  }
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
      freeWhere((where_clause **)&wb);
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
      if (ensureWhereValueSize(clause, clause->n_values + n_args) != 0) {
        freeWhere(&clause);
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

where_builder *nextWhere(where_builder *wb)
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

