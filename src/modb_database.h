#ifndef H__MODB_DATABASE__
#define H__MODB_DATABASE__

#include "database.h"
#include "modb_types.h"


// Connection reference
int modbUse(stored_conn *sconn, modb_ref *modb, int override);
int modbFindUse(stored_conn *sconn, modb_ref *modb);
void modbReleaseUse(stored_conn *sconn);


// MODB instance
int modbCreate(stored_conn *sconn, modb_ref *modb, column_data **col_data, size_t n_cols);
int modbExists(stored_conn *sconn, modb_ref *modb);
int modbHasExtendedMetadata(stored_conn *sconn, modb_ref *modb);
int modbDestroy(stored_conn *sconn, modb_ref *modb);


#endif // H__MODB_DATABASE__
