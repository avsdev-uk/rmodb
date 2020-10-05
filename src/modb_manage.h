#ifndef H__MODB_MANAGE__
#define H__MODB_MANAGE__

#include "database.h"
#include "modb_types.h"


// Connection reference
int modbUse(struct stored_conn_t *sconn, struct modb_t *modb, int override);
int modbFindUse(struct stored_conn_t *sconn, struct modb_t *modb);
void modbReleaseUse(struct stored_conn_t *sconn);


// MODB instance
int modbCreate(struct stored_conn_t *sconn, struct modb_t *modb);
int modbExists(struct stored_conn_t *sconn, struct modb_t *modb);
int modbDestroy(struct stored_conn_t *sconn, struct modb_t *modb);

int modbAccountingCreate(struct stored_conn_t *sconn, struct modb_t *modb);
int modbAccountingExists(struct stored_conn_t *sconn, struct modb_t *modb);
int modbAccountingDestroy(struct stored_conn_t *sconn, struct modb_t *modb);

int modbMetaExtCreate(struct stored_conn_t *sconn, struct modb_t *modb,
                      struct column_data_t **col_data, size_t cols);
int modbMetaExtExists(struct stored_conn_t *sconn, struct modb_t *modb);
int modbMetaExtDestroy(struct stored_conn_t *sconn, struct modb_t *modb);


#endif // H__MODB_MANAGE__
