#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <mysql.h>

#include "database.h"


struct stored_conn_t *storedConnections = 0;
int nextConnId = 1;
int defaultTimeout = -1;


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

  while (ptr != 0) {
    if (ptr->name != 0 && strcmp(ptr->name, name) == 0) {
      return ptr;
    }
    ptr = ptr->next;
  }

  return 0;
}


struct stored_conn_t *createStoredConnection(const char *name)
{
  struct stored_conn_t *sconn, *tail;

  if (name && strlen(name) > 0) {
    if (connectionByName(name) != 0) {
      fprintf(stderr, "createStoredConnection: Named connection already exists\n");
      return 0;
    }
  }

  sconn = (struct stored_conn_t *)malloc(sizeof(struct stored_conn_t));
  if (sconn == 0) {
    fprintf(stderr, "malloc: (%d) %s\n", errno, strerror(errno));
    return 0;
  }

  sconn->conn_id = nextConnId++;
  sconn->name = 0;
  sconn->conn = (MYSQL *)malloc(sizeof(MYSQL));
  sconn->isOpen = 0;
  sconn->isTransact = 0;
  sconn->needsReset = 0;
  sconn->timeout = -1;
  sconn->prev = 0;
  sconn->next = 0;

  if (sconn->conn == 0) {
    fprintf(stderr, "malloc: (%d) %s\n", errno, strerror(errno));
    destroyStoredConnection(sconn);
    return 0;
  }

  if (name && strlen(name) > 0) {
    sconn->name = (char *)malloc(sizeof(char) * strlen(name));
    if (sconn->name == 0) {
      fprintf(stderr, "malloc: (%d) %s\n", errno, strerror(errno));
      destroyStoredConnection(sconn);
      return 0;
    }
    memcpy(sconn->name, name, strlen(name) * sizeof(char));
  }

  if (mysql_init(sconn->conn) == 0) {
    fprintf(stderr, "mysql_init: unknown error\n");
    destroyStoredConnection(sconn);
    return 0;
  }

  if (defaultTimeout >= 0) {
    setTimeout(sconn, defaultTimeout);
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
    fprintf(stderr, "malloc: (%d) %s\n", errno, strerror(errno));
    return 0;
  }

  if (mysql_init(new_conn) == 0) {
    fprintf(stderr, "mysql_init: unknown error\n");
    return 0;
  }

  free(sconn->conn);
  sconn->conn = new_conn;

  if (sconn->timeout >= 0) {
    setTimeout(sconn, sconn->timeout);
  }

  sconn->needsReset = 0;

  return sconn;
}
void destroyStoredConnection(struct stored_conn_t *sconn)
{
  if (sconn->isTransact) {
    // rollback transaction
  }

  if (sconn->isOpen) {
    mysql_close(sconn->conn);
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


int setTimeout(struct stored_conn_t *sconn, unsigned int timeout)
{
  sconn->timeout = timeout;

  if (mysql_optionsv(sconn->conn, MYSQL_OPT_CONNECT_TIMEOUT, (void *)&timeout) != 0) {
    return -1;
  }
  if (mysql_optionsv(sconn->conn, MYSQL_OPT_READ_TIMEOUT, (void *)&timeout) != 0) {
    return -1;
  }
  if (mysql_optionsv(sconn->conn, MYSQL_OPT_WRITE_TIMEOUT, (void *)&timeout) != 0) {
    return -1;
  }

  return 0;
}
void setDefaultTimeout(unsigned int timeout)
{
  defaultTimeout = timeout;
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
    fprintf(stderr, "connectToHost: Connection already open\n");
    return -1;
  }

  if (sconn->needsReset && resetStoredConnection(sconn) != sconn) {
    return -1;
  }

  ret = mysql_real_connect(sconn->conn, host, user, passwd, db, port, NULL, 0);
  // flags: CLIENT_MULTI_STATEMENTS

  if (ret == 0) {
    fprintf(
      stderr, "mysql_real_connect: (%d) %s\n", mysql_errno(sconn->conn), mysql_error(sconn->conn)
    );
    sconn->needsReset = 1;
    errno = -mysql_errno(sconn->conn);
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
    fprintf(stderr, "connectToSocket: Connection already open\n");
    return -1;
  }

  if (sconn->needsReset && resetStoredConnection(sconn) != sconn) {
    return -1;
  }

  ret = mysql_real_connect(sconn->conn, NULL, user, passwd, db, 0, unix_socket, 0);
  // flags: CLIENT_MULTI_STATEMENTS

  if (ret == 0) {
    fprintf(
      stderr, "mysql_real_connect: (%d) %s\n", mysql_errno(sconn->conn), mysql_error(sconn->conn)
    );
    sconn->needsReset = 1;
    errno = -mysql_errno(sconn->conn);
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
  if (sconn->isTransact) {
    // rollback transaction
  }

  if (sconn->isOpen) {
    mysql_close(sconn->conn);
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