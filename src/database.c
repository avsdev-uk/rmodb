#include <mysql.h>

#include "database.h"

void sessionEnd()
{
  destroyAllConnections();
  mysql_library_end();
  setDefaultTimeout((unsigned int)-1);
}
