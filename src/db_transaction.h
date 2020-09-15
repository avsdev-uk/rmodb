#ifndef H__DB_TRANSACTION__
#define H__DB_TRANSACTION__

#include "db_connection.h"

// Transaction management
int transactionStart(struct stored_conn_t *sconn);
int transactionCommit(struct stored_conn_t *sconn);
int transactionRollback(struct stored_conn_t *sconn);

#endif // H__DB_TRANSACTION__
