#ifndef H__DB_QUERY__
#define H__DB_QUERY__

#include <stdint.h>
#include <stddef.h>

#include "db_connection.h"
#include "db_column.h"
#include "db_where-builder.h"

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


// Where query methods
int64_t countQuery(struct stored_conn_t *sconn,
                   const char *table, size_t table_len, where_builder *wb);
int64_t updateQuery(struct stored_conn_t *sconn,
                    const char *table, size_t table_len,
                    const char *set, size_t set_len, where_builder *wb);
int64_t deleteQuery(struct stored_conn_t *sconn,
                    const char *table, size_t table_len, where_builder *wb);

int64_t updateByIdQuery(struct stored_conn_t *sconn,
                        const char *table, size_t table_len,
                        const char *set, size_t set_len,
                        const char *id_col, unsigned int id);
int64_t softDeleteByIdQuery(struct stored_conn_t *sconn,
                            const char *table, size_t table_len,
                            const char *id_col, unsigned int id);
int64_t deleteByIdQuery(struct stored_conn_t *sconn,
                        const char *table, size_t table_len, const char *id_col, unsigned int id);


// Map helpers
int64_t syncIdMap(struct stored_conn_t *sconn, const char *table, size_t table_len,
                  const char *primary_col, const char *map_col,
                  unsigned int primary_id, size_t n_maps, unsigned int *map_ids);
int64_t syncIdMap_va(struct stored_conn_t *sconn, const char *table, size_t table_len,
                     const char *primary_col, const char *map_col,
                     unsigned int primary_id, size_t n_maps, va_list args);

int64_t hasIdMap(struct stored_conn_t *sconn, const char *table, size_t table_len,
                 const char *primary_col, const char *map_col,
                 unsigned int primary_id, unsigned int map_id);
int64_t addIdMap(struct stored_conn_t *sconn, const char *table, size_t table_len,
                 const char *primary_col, const char *map_col,
                 unsigned int primary_id, unsigned int map_id);
int64_t removeIdMap(struct stored_conn_t *sconn, const char *table, size_t table_len,
                    const char *primary_col, const char *map_col,
                    unsigned int primary_id, unsigned int map_id);


#endif // H__DB_QUERY__
