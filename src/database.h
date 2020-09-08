#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <mysql.h>

struct stored_conn_t {
  int conn_id;
  char *name;

  MYSQL *conn;

  int isOpen     :1;
  int isTransact :1;
  int needsReset :1;
  int __FLAGS;

  unsigned int timeout;

  struct stored_conn_t *prev;
  struct stored_conn_t *next;
};


struct stored_conn_t *connectionById(int conn_id);
struct stored_conn_t *connectionByName(const char *name);


struct stored_conn_t *createStoredConnection(const char *name);
struct stored_conn_t *resetStoredConnection(struct stored_conn_t *sconn);
void destroyStoredConnection(struct stored_conn_t *sconn);
void destroyAllConnections();

int connectionCount();

int setTimeout(struct stored_conn_t *sconn, unsigned int timeout);
void setDefaultTimeout(unsigned int timeout);


int connectToHost(struct stored_conn_t *sconn, 
                  const char *host, unsigned int port,
                  const char *user, const char *passwd, const char *db);
int connectToSocket(struct stored_conn_t *sconn,
                    const char *unix_socket,
                    const char *user, const char *passwd, const char *db);

void closeConnection(struct stored_conn_t *sconn);
void closeAllConnections();

#endif // __DATABASE_H__