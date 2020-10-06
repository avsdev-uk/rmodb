#ifndef H__DB_CONNECTION__
#define H__DB_CONNECTION__

#include <stdint.h>
#include <stdlib.h>

#define SQCONN(s) (MYSQL *)s->conn

struct stored_conn_t {
  int conn_id;
  char *name;
  size_t name_len;

  void *conn;

  char *last_qry;
  size_t last_qry_len;
  size_t last_qry_alloc;

  uint32_t num_queries;

  uint8_t isOpen        :1;
  uint8_t inTransaction :1;
  uint8_t needsReset    :1;

  unsigned int timeout;

  struct stored_conn_t *prev;
  struct stored_conn_t *next;
};
typedef struct stored_conn_t stored_conn;

// Connections are stored in a linked list, these functions provide access
struct stored_conn_t *connectionById(int conn_id);
struct stored_conn_t *connectionByName(const char *name);

int connectionCount(void);

// Connection management
struct stored_conn_t *createStoredConnection(const char *name);
struct stored_conn_t *resetStoredConnection(struct stored_conn_t *sconn);
void destroyStoredConnection(struct stored_conn_t *sconn);
void destroyAllConnections(void);

int connectToHost(struct stored_conn_t *sconn,
                  const char *host, unsigned int port,
                  const char *user, const char *passwd, const char *db);
int connectToSocket(struct stored_conn_t *sconn,
                    const char *unix_socket,
                    const char *user, const char *passwd, const char *db);
void closeConnection(struct stored_conn_t *sconn);
void closeAllConnections(void);

#endif // H__DB_CONNECTION__
