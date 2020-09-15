#ifndef __DB_TIMEOUT_H__
#define __DB_TIMEOUT_H__

#include "db_connection.h"

// Timeouts can either be set globally or on a per-connection basis
void setDefaultTimeout(unsigned int timeout);
unsigned int getDefaultTimeout();

int setTimeout(struct stored_conn_t *sconn, unsigned int timeout);

#endif // __DB_TIMEOUT_H__