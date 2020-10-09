#ifndef H__MODB_OBJECTS__
#define H__MODB_OBJECTS__

#include "database.h"
#include "modb_ref.h"


// Object object
struct object_t {
  unsigned int id;

  char *data;
  size_t data_len;
};

struct object_t *allocObject(void);
struct object_t **allocObjects(size_t n_objects);
void freeObject(struct object_t **object);
void freeObjects(struct object_t ***objects_ptr, size_t n_objects);


// MODB Objects
int modbObjectById(stored_conn *sconn, modb_ref *modb, unsigned int id,
                   struct object_t **object);
// Objects by owner
// Objects by group
// Objects by owner/owner groups

int modbObjectList(stored_conn *sconn, modb_ref *modb, int with_deleted,
                   struct object_t ***objects, size_t *n_objects);

int64_t modbObjectCreate(stored_conn *sconn, modb_ref *modb,
                         unsigned int id, const char *data, size_t data_len);
int64_t modbObjectUpdate(stored_conn *sconn, modb_ref *modb, unsigned int id,
                         const char *data, size_t data_len);
int modbObjectDelete(stored_conn *sconn, modb_ref *modb, unsigned int id);
int modbObjectDestroy(stored_conn *sconn, modb_ref *modb, unsigned int id);

#endif // H__MODB_OBJECTS__
