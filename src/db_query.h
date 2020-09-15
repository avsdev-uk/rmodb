#ifndef H__DB_QUERY__
#define H__DB_QUERY__

#include <stdint.h>
#include "db_connection.h"

// Generic query methods
uint64_t simpleQuery(struct stored_conn_t *sconn, const char *qry, size_t qry_len);
uint64_t scalarQuery(struct stored_conn_t *sconn, const char *qry, size_t qry_len,
                     char **res, size_t *res_len);
uint64_t tableQuery(struct stored_conn_t *sconn, const char *qry, size_t qry_len, int scalar_result,
                    struct column_data_t ***col_data, size_t *n_cols);

// Scalar helpers
int scalarInt(struct stored_conn_t *sconn, const char *qry, size_t qry_len, int default_value);
unsigned int scalarUInt(struct stored_conn_t *sconn, const char *qry, size_t qry_len,
                        unsigned int default_value);
double scalarReal(struct stored_conn_t *sconn, const char *qry, size_t qry_len,
                  double default_value);
char scalarChar(struct stored_conn_t *sconn, const char *qry, size_t qry_len, char default_value);
char *scalarString(struct stored_conn_t *sconn, const char *qry, size_t qry_len,
                   char *default_value);

#endif // H__DB_QUERY__
