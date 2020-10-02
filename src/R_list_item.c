#include <string.h>

#include "R_list_item.h"

SEXP R_listItem(SEXP list, const char *name)
{
  SEXP r_names = Rf_getAttrib(list, R_NamesSymbol);
  SEXP r_name;

  for (int i = 0; i < Rf_length(list); i++) {
    if (TYPEOF(r_names) == STRSXP) {
      r_name = STRING_ELT(r_names, i);
    } else {
      r_name = VECTOR_ELT(r_names, i);
    }
    if (strcmp(Rf_translateCharUTF8(r_name), name) == 0) {
      return VECTOR_ELT(list, i);
    }
  }

  return R_NilValue;
}
