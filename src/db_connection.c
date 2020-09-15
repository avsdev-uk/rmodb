#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <mysql.h>

#include "db_connection.h"
#include "db_timeout.h"
#include "db_transaction.h"


static struct stored_conn_t *storedConnections = 0;
static int nextConnId = 1;


struct stored_conn_t *connectionById(int conn_id)
{
  struct stored_conn_t *ptr = storedConnections;

  while (ptr != 0) {
    if (ptr->conn_id == conn_id) {
      return ptr;
    }
    ptr = ptr->next;
  }

  return 0;
}
struct stored_conn_t *connectionByName(const char *name)
{
  struct stored_conn_t *ptr = storedConnections;

  size_t name_len = strlen(name);
  while (ptr != 0) {
    if (ptr->name != 0
      && strlen(ptr->name) == name_len
      && strncmp(name, ptr->name, name_len) == 0) {
      return ptr;
    }
    ptr = ptr->next;
  }

  return 0;
}

int connectionCount()
{
  int i = 0;
  struct stored_conn_t *ptr = storedConnections;

  while (ptr != 0) {
    i++;
    ptr = ptr->next;
  }

  return i;
}

struct stored_conn_t *createStoredConnection(const char *name)
{
  struct stored_conn_t *sconn, *tail;

  if (name && strlen(name) > 0) {
    if (connectionByName(name) != 0) {
      fprintf(stderr, "[%d]createStoredConnection: Named connection already exists\n", __LINE__);
      return 0;
    }
  }

  sconn = (struct stored_conn_t *)malloc(sizeof(struct stored_conn_t));
  if (sconn == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }

  sconn->conn_id = nextConnId++;
  sconn->name = 0;
  sconn->conn = (MYSQL *)malloc(sizeof(MYSQL));
  sconn->isOpen = 0;
  sconn->inTransaction = 0;
  sconn->needsReset = 0;
  sconn->timeout = (unsigned int)-1;
  sconn->prev = 0;
  sconn->next = 0;

  if (sconn->conn == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    destroyStoredConnection(sconn);
    return 0;
  }

  if (name && strlen(name) > 0) {
    sconn->name_len = sizeof(char) * strlen(name);
    sconn->name = (char *)malloc(sconn->name_len + 1);
    if (sconn->name == 0) {
      fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
      destroyStoredConnection(sconn);
      return 0;
    }
    memcpy(sconn->name, name, sconn->name_len);
    sconn->name[sconn->name_len] = '\0';
  }

  if (mysql_init(SQCONN(sconn)) == 0) {
    fprintf(stderr, "[%d]mysql_init: unknown error\n", __LINE__);
    destroyStoredConnection(sconn);
    return 0;
  }

  if (getDefaultTimeout() != (unsigned int)-1) {
    setTimeout(sconn, getDefaultTimeout());
  }

  if (storedConnections == 0) {
    storedConnections = sconn;
  } else {
    tail = storedConnections;
    while (tail->next != 0) {
      tail = tail->next;
    }
    tail->next = sconn;
    sconn->prev = tail;
  }

  return sconn;
}
struct stored_conn_t *resetStoredConnection(struct stored_conn_t *sconn)
{
  MYSQL *new_conn;

  new_conn = (MYSQL *)malloc(sizeof(MYSQL));
  if (new_conn == 0) {
    fprintf(stderr, "[%d]malloc: (%d) %s\n", __LINE__, errno, strerror(errno));
    return 0;
  }

  if (mysql_init(new_conn) == 0) {
    fprintf(stderr, "[%d]mysql_init: unknown error\n", __LINE__);
    return 0;
  }

  mysql_close(sconn->conn);
  free(sconn->conn);
  sconn->conn = new_conn;

  if (sconn->timeout != (unsigned int)-1) {
    setTimeout(sconn, sconn->timeout);
  }

  sconn->needsReset = 0;

  return sconn;
}
void destroyStoredConnection(struct stored_conn_t *sconn)
{
  if (sconn->isOpen) {
    closeConnection(sconn);
  }

  if (sconn->name) {
    free(sconn->name);
    sconn->name = 0;
  }

  if (sconn->conn) {
    free(sconn->conn);
    sconn->conn = 0;
  }

  if (sconn->prev) {
    sconn->prev->next = sconn->next;
  } else {
    storedConnections = sconn->next;
  }

  if (sconn->next) {
    sconn->next->prev = sconn->prev;
  }

  free(sconn);
}
void destroyAllConnections()
{
  struct stored_conn_t *ptr = storedConnections;

  while (ptr != 0 && ptr->next != 0) {
    ptr = ptr->next;
    destroyStoredConnection(ptr->prev);
  }

  if (ptr != 0) {
    destroyStoredConnection(ptr);
  }
}


int connectToHost(struct stored_conn_t *sconn,
                  const char *host, unsigned int port,
                  const char *user, const char *passwd, const char *db)
{
  MYSQL *ret;
  int freeCon = 0;

  if (sconn == NULL) {
    sconn = createStoredConnection(NULL);
    if (sconn == 0) {
      return -1;
    }
    freeCon = 1;
  }

  if (sconn->isOpen) {
    fprintf(stderr, "[%d]connectToHost: Connection already open\n", __LINE__);
    return -1;
  }

  if (sconn->needsReset && resetStoredConnection(sconn) != sconn) {
    return -1;
  }

  ret = mysql_real_connect(SQCONN(sconn), host, user, passwd, db, port, NULL, 0);
  // flags: CLIENT_MULTI_STATEMENTS

  if (ret == 0) {
    fprintf(
      stderr, "[%d]mysql_real_connect: (%d) %s\n",
      __LINE__, mysql_errno(SQCONN(sconn)), mysql_error(SQCONN(sconn))
    );
    sconn->needsReset = 1;
    errno = -(int)mysql_errno(SQCONN(sconn));
    if (freeCon) {
      destroyStoredConnection(sconn);
    }
    return errno;
  }

  sconn->isOpen = 1;

  return sconn->conn_id;
}
int connectToSocket(struct stored_conn_t *sconn,
                    const char *unix_socket,
                    const char *user, const char *passwd, const char *db)
{
  MYSQL *ret;
  int freeCon = 0;

  if (sconn == NULL) {
    sconn = createStoredConnection(NULL);
    if (sconn == 0) {
      return -1;
    }
    freeCon = 1;
  }

  if (sconn->isOpen) {
    fprintf(stderr, "[%d]connectToSocket: Connection already open\n", __LINE__);
    return -1;
  }

  if (sconn->needsReset && resetStoredConnection(sconn) != sconn) {
    return -1;
  }

  ret = mysql_real_connect(SQCONN(sconn), NULL, user, passwd, db, 0, unix_socket, 0);
  // flags: CLIENT_MULTI_STATEMENTS

  if (ret == 0) {
    fprintf(
      stderr, "[%d]mysql_real_connect: (%d) %s\n",
      __LINE__, mysql_errno(SQCONN(sconn)), mysql_error(SQCONN(sconn))
    );
    sconn->needsReset = 1;
    errno = -(int)mysql_errno(SQCONN(sconn));
    if (freeCon) {
      destroyStoredConnection(sconn);
    }
    return errno;
  }

  sconn->isOpen = 1;

  return sconn->conn_id;
}

void closeConnection(struct stored_conn_t *sconn)
{
  if (sconn->inTransaction) {
    transactionRollback(sconn);
  }

  if (sconn->isOpen) {
    mysql_close(SQCONN(sconn));
  }

  sconn->needsReset = 1;
  sconn->isOpen = 0;
}

void closeAllConnections()
{
  struct stored_conn_t *c, *ptr = storedConnections;

  while (ptr != 0) {
    c = ptr;
    ptr = ptr->next;
    destroyStoredConnection(c);
  }
}
