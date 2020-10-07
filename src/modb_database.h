#ifndef H__MODB_MANAGEMENT__
#define H__MODB_MANAGEMENT__

#include "database.h"
#include "modb_types.h"


// Connection reference
int modbUse(stored_conn *sconn, modb_ref *modb, int override);
int modbFindUse(stored_conn *sconn, modb_ref *modb);
void modbReleaseUse(stored_conn *sconn);


// MODB instance
int modbCreate(stored_conn *sconn, modb_ref *modb);
int modbExists(stored_conn *sconn, modb_ref *modb);
int modbDestroy(stored_conn *sconn, modb_ref *modb);

int modbAccountingCreate(stored_conn *sconn, modb_ref *modb);
int modbAccountingExists(stored_conn *sconn, modb_ref *modb);
int modbAccountingDestroy(stored_conn *sconn, modb_ref *modb);

int modbMetaExtCreate(stored_conn *sconn, modb_ref *modb,
                      column_data **col_data, size_t cols);
int modbMetaExtExists(stored_conn *sconn, modb_ref *modb);
int modbMetaExtDestroy(stored_conn *sconn, modb_ref *modb);


#endif // H__MODB_MANAGEMENT__
