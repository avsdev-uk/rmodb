#include <mysql.h>

#include "db_timeout.h"


int defaultTimeout = -1;


void setDefaultTimeout(unsigned int timeout)
{
  defaultTimeout = timeout;
}
unsigned int getDefaultTimeout()
{
  return defaultTimeout;
}

int setTimeout(struct stored_conn_t *sconn, unsigned int timeout)
{
  sconn->timeout = timeout;

  if (mysql_optionsv(SQCONN(sconn), MYSQL_OPT_CONNECT_TIMEOUT, (void *)&timeout) != 0) {
    return -1;
  }
  if (mysql_optionsv(SQCONN(sconn), MYSQL_OPT_READ_TIMEOUT, (void *)&timeout) != 0) {
    return -1;
  }
  if (mysql_optionsv(SQCONN(sconn), MYSQL_OPT_WRITE_TIMEOUT, (void *)&timeout) != 0) {
    return -1;
  }

  return 0;
}