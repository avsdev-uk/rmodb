#ifndef H__DB_TIMEOUT__
#define H__DB_TIMEOUT__

#include "db_connection.h"

// Timeouts can either be set globally or on a per-connection basis
void setDefaultTimeout(unsigned int timeout);
unsigned int getDefaultTimeout(void);

int setTimeout(struct stored_conn_t *sconn, unsigned int timeout);

#endif // H__DB_TIMEOUT__
