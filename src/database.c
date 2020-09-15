#include <mysql.h>

#include "database.h"

void sessionEnd()
{
  destroyAllConnections();
  mysql_library_end();
  setDefaultTimeout(-1);
}