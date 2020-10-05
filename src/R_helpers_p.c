#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "R_helpers_p.h"
#include "strext.h"

struct stored_conn_t *getConnectionByRef(SEXP r_conn_ref)
{
  struct stored_conn_t *sconn = 0;

  if (Rf_isString(r_conn_ref)) {
    sconn = connectionByName(Rf_translateCharUTF8(STRING_ELT(r_conn_ref, 0)));
  } else if (Rf_isInteger(r_conn_ref)) {
    sconn = connectionById(Rf_asInteger(r_conn_ref));
  } else {
    Rf_error("Neither name or id provided");
  }

  return sconn;
}
