/****************************************************************************************/
/* Program Name: idcapp.h, this program is the declaration file for common functions    */
/*              and classes of the data center project.                                 */
/****************************************************************************************/

#ifndef IDCAPP_H
#define IDCAPP_H

#include "_public.h"
#include "_mysql.h"

struct st_zhobtmind
{
  char obtid[11];      // Stock code.
  char []
};

// Class for operating national station minute observation data.
class CZHOBTMIND
{
public:
  connection *m_conn;     // Database connection.
  CLogFile *m_logfile;    // Log file.

  sqlstatement m_stmt;    // SQL for inserting into the table.

  char m_buffer[1024];    // One line read from the file.
  struct st_zhobtmind m_zhobtmind; // Structure for national station minute observation data.

  CZHOBTMIND();
  CZHOBTMIND(connection *conn, CLogFile *logfile);

  ~CZHOBTMIND();

  void BindConnLog(connection *conn, CLogFile *logfile); // Pass connection and CLogFile.
  bool SplitBuffer(char *strBuffer, bool bisxml); // Split one line read from the file into m_zhobtmind structure.
  bool InsertTable(); // Insert data from m_zhobtmind structure into the T_ZHOBTMIND table.
};

#endif
