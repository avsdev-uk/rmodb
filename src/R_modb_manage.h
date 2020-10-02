#ifndef H__R_MODB_MANAGE__
#define H__R_MODB_MANAGE__

#include <Rinternals.h>

SEXP modb_connectionInfo(SEXP r_conn_ref);

SEXP modb_connectToHost(SEXP r_name, SEXP r_host, SEXP r_port,
                        SEXP r_username, SEXP r_password, SEXP r_database);
SEXP modb_connectToSocket(SEXP r_name, SEXP r_socket,
                          SEXP r_username, SEXP r_password, SEXP r_database);

SEXP modb_disconnect(SEXP r_conn_ref);


SEXP modb_exists(SEXP r_conn_ref, SEXP r_name);
SEXP modb_create(SEXP r_conn_ref, SEXP r_name, SEXP r_extra_meta);
SEXP modb_destroy(SEXP r_conn_ref, SEXP r_name);

#endif // H__R_MODB_MANAGE__
