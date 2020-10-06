#include <string.h>

#include "R_modb_manage.h"
#include "R_list_item.h"
#include "modb_manage.h"

#include "R_helpers_p.h"


SEXP modb_connectionInfo(SEXP r_conn_ref)
{
  struct stored_conn_t *sconn;
  SEXP res, conn_name, last_qry, names;

  if ((sconn = getConnectionByRef(r_conn_ref)) == 0) {
    return R_NilValue;
  }

  conn_name = PROTECT(Rf_allocVector(STRSXP, 1));
  SET_STRING_ELT(conn_name, 0, PROTECT(Rf_mkChar(sconn->name)));

  last_qry = PROTECT(Rf_allocVector(STRSXP, 1));
  if (sconn->last_qry != 0) {
    SET_STRING_ELT(last_qry, 0, PROTECT(Rf_mkChar(sconn->last_qry)));
  } else {
    SET_STRING_ELT(last_qry, 0, PROTECT(NA_STRING));
  }

  res = PROTECT(Rf_allocVector(VECSXP, 4));
  SET_VECTOR_ELT(res, 0, PROTECT(Rf_ScalarInteger(sconn->conn_id)));
  SET_VECTOR_ELT(res, 1, conn_name);
  SET_VECTOR_ELT(res, 2, last_qry);
  SET_VECTOR_ELT(res, 3, PROTECT(Rf_ScalarInteger((int)sconn->num_queries)));

  names = PROTECT(Rf_allocVector(STRSXP, 4));
  SET_STRING_ELT(names, 0, PROTECT(Rf_mkChar("id")));
  SET_STRING_ELT(names, 1, PROTECT(Rf_mkChar("name")));
  SET_STRING_ELT(names, 2, PROTECT(Rf_mkChar("num_queries")));
  SET_STRING_ELT(names, 3, PROTECT(Rf_mkChar("last_query")));
  Rf_setAttrib(res, R_NamesSymbol, names);

  UNPROTECT(12);
  return res;
}

SEXP modb_connectToHost(SEXP r_name, SEXP r_host, SEXP r_port,
                        SEXP r_username, SEXP r_password, SEXP r_database)
{
  struct stored_conn_t *sconn;
  const char *name = 0, *host, *user, *pass, *db;
  unsigned int port;

  if (!Rf_isNull(r_name)) {
    name = Rf_translateCharUTF8(STRING_ELT(r_name, 0));
  }

  host = Rf_translateCharUTF8(STRING_ELT(r_host, 0));
  port = (unsigned int)Rf_asInteger(r_port);
  user = Rf_translateCharUTF8(STRING_ELT(r_username, 0));
  pass = Rf_translateCharUTF8(STRING_ELT(r_password, 0));
  db = Rf_translateCharUTF8(STRING_ELT(r_database, 0));

  sconn = createStoredConnection(name);
  if (sconn == 0) {
    return R_NilValue;
  }

  if (connectToHost(sconn, host, port, user, pass, db) < 0) {
    destroyStoredConnection(sconn);
    return R_NilValue;
  }

  return Rf_ScalarInteger(sconn->conn_id);
}
SEXP modb_connectToSocket(SEXP r_name, SEXP r_socket,
                          SEXP r_username, SEXP r_password, SEXP r_database)
{
  struct stored_conn_t *sconn;
  const char *name = 0, *sock, *user, *pass, *db;

  if (!Rf_isNull(r_name)) {
    name = Rf_translateCharUTF8(STRING_ELT(r_name, 0));
  }

  sock = Rf_translateCharUTF8(STRING_ELT(r_socket, 0));
  user = Rf_translateCharUTF8(STRING_ELT(r_username, 0));
  pass = Rf_translateCharUTF8(STRING_ELT(r_password, 0));
  db = Rf_translateCharUTF8(STRING_ELT(r_database, 0));

  sconn = createStoredConnection(name);
  if (sconn == 0) {
    return R_NilValue;
  }

  if (connectToSocket(sconn, sock, user, pass, db) < 0) {
    destroyStoredConnection(sconn);
    return R_NilValue;
  }

  return Rf_ScalarInteger(sconn->conn_id);
}
SEXP modb_disconnect(SEXP r_conn_ref)
{
  struct stored_conn_t *sconn;
  int conn_id;

  if ((sconn = getConnectionByRef(r_conn_ref)) == 0) {
    return R_NilValue;
  }

  conn_id = sconn->conn_id;

  modbReleaseUse(sconn);
  closeConnection(sconn);
  destroyStoredConnection(sconn);

  return Rf_ScalarInteger(conn_id);
}


SEXP modb_exists(SEXP r_conn_ref, SEXP r_name)
{
  struct stored_conn_t *sconn;
  struct modb_t modb;

  if ((sconn = getConnectionByRef(r_conn_ref)) == 0) {
    Rf_error("invalid connection reference\n");
  }

  modb.name = Rf_translateCharUTF8(STRING_ELT(r_name, 0));
  modb.name_len = strlen(modb.name);

  return Rf_ScalarLogical(modbExists(sconn, &modb));
}

SEXP modb_create(SEXP r_conn_ref, SEXP r_name, SEXP r_extra_meta)
{
  struct stored_conn_t *sconn;
  struct modb_t modb;
  struct column_data_t **cols;
  size_t n_cols;
  SEXP r_col, r_col_name, r_col_type, r_col_null;

  if ((sconn = getConnectionByRef(r_conn_ref)) == 0) {
    Rf_error("invalid connection reference\n");
  }

  modb.name = Rf_translateCharUTF8(STRING_ELT(r_name, 0));
  modb.name_len = strlen(modb.name);

  if (modbExists(sconn, &modb)) {
    Rf_warning("an MODB instance named '%s' already exists\n", modb.name);
    return Rf_ScalarLogical(FALSE);
  }

  if (!modbCreate(sconn, &modb)) {
    modb_destroy(r_conn_ref, r_name);
    Rf_error("failed to create MODB instance");
  }
  if (!modbAccountingCreate(sconn, &modb)) {
    modb_destroy(r_conn_ref, r_name);
    Rf_error("failed to create MODB instance");
  }
  if (!Rf_isNull(r_extra_meta)) {
    n_cols = (size_t)Rf_length(r_extra_meta);
    cols = (struct column_data_t **)calloc(sizeof(struct column_data_t *), n_cols);
    if (cols == 0) {
      modb_destroy(r_conn_ref, r_name);
      Rf_error("failed to create MODB instance");
    }

    for (size_t i = 0; i < n_cols; i++) {
      r_col = VECTOR_ELT(r_extra_meta, (int)i);
      r_col_name = STRING_ELT(R_listItem(r_col, "name"), 0);
      r_col_type = R_listItem(r_col, "type");
      r_col_null = R_listItem(r_col, "nullable");

      *(cols + i) = initEmptyColumn(
            (e_column_type)(unsigned int)Rf_asInteger(r_col_type),
            Rf_asLogical(r_col_null),
            Rf_translateCharUTF8(r_col_name),
            0, 0, 0);
      if (*(cols + i) == 0) {
        freeColumns(cols, i);
        modb_destroy(r_conn_ref, r_name);
        Rf_error("failed to create MODB instance");
      }
    }

    if (!modbMetaExtCreate(sconn, &modb, cols, n_cols)) {
      freeColumns(cols, n_cols);
      modb_destroy(r_conn_ref, r_name);
      Rf_error("failed to create MODB instance");
    }

    freeColumns(cols, n_cols);
  }

  return Rf_ScalarLogical(TRUE);
}
SEXP modb_destroy(SEXP r_conn_ref, SEXP r_name)
{
  struct stored_conn_t *sconn;
  struct modb_t modb;

  if ((sconn = getConnectionByRef(r_conn_ref)) == 0) {
    Rf_error("invalid connection reference\n");
  }

  modb.name = Rf_translateCharUTF8(STRING_ELT(r_name, 0));
  modb.name_len = strlen(modb.name);

  if (modbMetaExtExists(sconn, &modb)) {
    modbMetaExtDestroy(sconn, &modb);
  }
  modbAccountingDestroy(sconn, &modb);
  modbDestroy(sconn, &modb);

  return Rf_ScalarLogical(TRUE);
}


SEXP modb_use(SEXP r_conn_ref, SEXP r_name, SEXP r_override)
{
  struct stored_conn_t *sconn;
  struct modb_t modb;

  if ((sconn = getConnectionByRef(r_conn_ref)) == 0) {
    Rf_error("invalid connection reference\n");
  }

  modb.name = Rf_translateCharUTF8(STRING_ELT(r_name, 0));
  modb.name_len = strlen(modb.name);

  if (!modbExists(sconn, &modb)) {
    Rf_error("an MODB instance named '%s' does not exist\n", modb.name);
  }

  if (!modbUse(sconn, &modb, Rf_asLogical(r_override))) {
    return Rf_ScalarLogical(FALSE);
  }

  return Rf_ScalarLogical(TRUE);
}
