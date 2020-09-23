#include <mysql.h>

#include "database.h"

void dbSessionEnd()
{
  destroyAllConnections();
  mysql_library_end();
  setDefaultTimeout((unsigned int)-1);
}
