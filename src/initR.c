#include <Rinternals.h>

#include "R_modb_manage.h"

static const R_CallMethodDef callMethods[] = {
  {"modb_connectionInfo", (DL_FUNC) &modb_connectionInfo, 1},
  {"modb_connectToHost", (DL_FUNC) &modb_connectToHost, 6},
  {"modb_connectToSocket", (DL_FUNC) &modb_connectToSocket, 5},
  {"modb_disconnect", (DL_FUNC) &modb_disconnect, 1},

  {"modb_exists", (DL_FUNC) &modb_exists, 2},
  {"modb_create", (DL_FUNC) &modb_create, 3},
  {"modb_destroy", (DL_FUNC) &modb_destroy, 2},

  {"modb_use", (DL_FUNC) &modb_use, 2},
  {NULL, NULL, 0}
};

void R_init_rmodb(DllInfo *dll)
{
  R_registerRoutines(dll, NULL, callMethods, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
  R_forceSymbols(dll, TRUE);
}
