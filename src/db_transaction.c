#include <stdio.h>
#include <mysql.h>

#include "db_transaction.h"

int transactionStart(struct stored_conn_t *sconn)
{
  if (mysql_autocommit(SQCONN(sconn), 1) != 0) {
    fprintf(
      stderr, "[%d]mysql_autocommit: (%d) %s\n",
      __LINE__, mysql_errno(SQCONN(sconn)), mysql_error(SQCONN(sconn))
    );
    return -1;
  }
  sconn->inTransaction = 1;
  return 0;
}
int transactionCommit(struct stored_conn_t *sconn)
{
  if (mysql_commit(SQCONN(sconn)) != 0) {
    fprintf(
      stderr, "[%d]mysql_commit: (%d) %s\n",
      __LINE__, mysql_errno(SQCONN(sconn)), mysql_error(SQCONN(sconn))
    );
    return -1;
  }
  if (mysql_autocommit(SQCONN(sconn), 0) != 0) {
    fprintf(
      stderr, "[%d]mysql_autocommit: (%d) %s\n",
      __LINE__, mysql_errno(SQCONN(sconn)), mysql_error(SQCONN(sconn))
    );
    return -1;
  }
  sconn->inTransaction = 0;
  return 0;
}
int transactionRollback(struct stored_conn_t *sconn)
{
  if (mysql_rollback(SQCONN(sconn)) != 0) {
    fprintf(
      stderr, "[%d]mysql_rollback: (%d) %s\n",
      __LINE__, mysql_errno(SQCONN(sconn)), mysql_error(SQCONN(sconn))
    );
    return -1;
  }
  if (mysql_autocommit(SQCONN(sconn), 0) != 0) {
    fprintf(
      stderr, "[%d]mysql_autocommit: (%d) %s\n",
      __LINE__, mysql_errno(SQCONN(sconn)), mysql_error(SQCONN(sconn))
    );
    return -1;
  }
  sconn->inTransaction = 0;
  return 0;
}