/*
 * Program Name: migratetable.cpp
 * Description: This program is a common module for data center, used to migrate data in tables.
 */

#include "_tools.h"

struct st_arg
{
  char connstr[101];     // Database connection parameters.
  char srctname[31];     // Table name to be migrated.
  char dsttname[31];     // Destination table name.
  char keycol[31];       // Unique key field name of the source table.
  char where[1001];      // Conditions for migrating data.
  char starttime[31];    // Program running time interval.
  int maxcount;          // Maximum number of records per migration operation.
  int timeout;           // Timeout during program execution.
  char pname[51];        // Process name during program execution.
} starg;

// Display program help
void _help(char *argv[]);

// Parse xml and populate the starg structure with parameters
bool _xmltoarg(char *strxmlbuffer);

CLogFile logfile;

// Check if the current time is within the program running time interval.
bool instarttime();

connection conn1;     // Database connection for executing query SQL statements.
connection conn2;     // Database connection for executing insert and delete SQL statements.

// Main business processing function.
bool _migratetable();

void EXIT(int sig);

CPActive PActive;

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    _help(argv);
    return -1;
  }

  // Close all signals and input/output
  // Handle signals for program exit
  CloseIOAndSignal();
  signal(SIGINT, EXIT);
  signal(SIGTERM, EXIT);

  if (logfile.Open(argv[1], "a+") == false)
  {
    printf("Failed to open log file (%s).\n", argv[1]);
    return -1;
  }

  // Parse xml and populate the starg structure with parameters
  if (_xmltoarg(argv[2]) == false)
    return -1;

  // Check if the current time is within the program running time interval.
  if (instarttime() == false)
    return 0;

  PActive.AddPInfo(starg.timeout, starg.pname);
  // Note: During program debugging, you can enable the following code to prevent timeout.
  // PActive.AddPInfo(starg.timeout * 100, starg.pname);

  if (conn1.connecttodb(starg.connstr, NULL) != 0)
  {
    logfile.Write("Failed to connect to the database (%s).\n%s\n", starg.connstr, conn1.m_cda.message);
    EXIT(-1);
  }

  if (conn2.connecttodb(starg.connstr, NULL) != 0)
  {
    logfile.Write("Failed to connect to the database (%s).\n%s\n", starg.connstr, conn2.m_cda.message);
    EXIT(-1);
  }

  // Main business processing function.
  _migratetable();
}

// Display program help
void _help(char *argv[])
{
  printf("Using: /project/tools1/bin/migratetable logfilename xmlbuffer\n\n");
  printf("Sample: /project/tools1/bin/procctl 3600 /project/tools1/bin/migratetable /log/idc/migratetable_ZHOBTMIND.log \"<connstr>127.0.0.1,root,mysqlpwd,mysql,3306</connstr><srctname>T_ZHOBTMIND</srctname><dsttname>T_ZHOBTMIND_HIS</dsttname><keycol>keyid</keycol><where>where ddatetime<timestampadd(minute,-120,now())</where><starttime>01,02,03,04,05,13</starttime><maxcount>300</maxcount><timeout>120</timeout><pname>migratetable_ZHOBTMIND</pname>\"\n\n");
  printf("This program is a common module for data center, used to migrate data in tables.\n");
  printf("logfilename: The log file for program running.\n");
  printf("xmlbuffer: The program parameters in XML format, as follows:\n\n");
  printf("connstr: Database connection parameters, format: ip,username,password,dbname,port.\n");
  printf("srctname: The table name of the source data to be migrated.\n");
  printf("dsttname: The table name of the destination table to be migrated to. Note that srctname and dsttname must have identical structures.\n");
  printf("keycol: The unique key field name of the source data table.\n");
  printf("starttime: The program running time interval, e.g., 02,13 means the program runs at 02:00 and 13:00, and does not run at other times.\n"\
         "           If starttime is empty, this parameter will be ignored, and the data migration will be executed whenever the program is started.\n"\
         "           To reduce the load on the database, data migration is generally performed during the database's idle time.\n");
  printf("where: The condition that the data to be migrated needs to meet, i.e., the 'where' part of the SQL statement.\n");
  printf("maxcount: The maximum number of records per migration operation, cannot exceed MAXPARAMS(256).\n");
  printf("timeout: The timeout for this program in seconds, recommended to set above 120.\n");
  printf("pname: The process name, preferably a name that is easy to understand and different from other processes, facilitating troubleshooting.\n\n");
}

// Parse xml and populate the starg structure with parameters
bool _xmltoarg(char *strxmlbuffer)
{
  memset(&starg, 0, sizeof(struct st_arg));

  GetXMLBuffer(strxmlbuffer, "connstr", starg.connstr, 100);
  if (strlen(starg.connstr) == 0)
  {
    logfile.Write("connstr is null.\n");
    return false;
  }

  GetXMLBuffer(strxmlbuffer, "srctname", starg.srctname, 30);
  if (strlen(starg.srctname) == 0)
  {
    logfile.Write("srctname is null.\n");
    return false;
  }

  GetXMLBuffer(strxmlbuffer, "dsttname", starg.dsttname, 30);
  if (strlen(starg.dsttname) == 0)
  {
    logfile.Write("dsttname is null.\n");
    return false;
  }

  GetXMLBuffer(strxmlbuffer, "keycol", starg.keycol, 30);
  if (strlen(starg.keycol) == 0)
  {
    logfile.Write("keycol is null.\n");
    return false;
  }

  GetXMLBuffer(strxmlbuffer, "where", starg.where, 1000);
  if (strlen(starg.where) == 0)
  {
    logfile.Write("where is null.\n");
    return false;
  }

  GetXMLBuffer(strxmlbuffer, "starttime", starg.starttime, 30);

  GetXMLBuffer(strxmlbuffer, "maxcount", &starg.maxcount);
  if (starg.maxcount == 0)
  {
    logfile.Write("maxcount is null.\n");
    return false;
  }
  if (starg.maxcount > MAXPARAMS)
    starg.maxcount = MAXPARAMS;

  GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);
  if (starg.timeout == 0)
  {
    logfile.Write("timeout is null.\n");
    return false;
  }

  GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50);
  if (strlen(starg.pname) == 0)
  {
    logfile.Write("pname is null.\n");
    return false;
  }

  return true;
}


void EXIT(int sig)
{
  logfile.Write("Program exited, sig=%d\n\n", sig);

  conn1.disconnect();
  conn2.disconnect();

  exit(0);
}

// Main business processing function.
bool _migratetable()
{
  CTimer Timer;

  // Get all column names from the data dictionary.
  CTABCOLS TABCOLS;

  // Get the column names of the table to be migrated from either starg.srctname or starg.dsttname.
  if (TABCOLS.allcols(&conn2, starg.dsttname) == false)
  {
    logfile.Write("Table %s does not exist.\n", starg.dsttname);
    return false;
  }

  char tmpvalue[51];    // The value of the key field to be migrated from the data source table.

  // Extract the unique keys of the records to be migrated from the data source table, no need to sort.
  sqlstatement stmtsel(&conn1);
  stmtsel.prepare("select %s from %s %s", starg.keycol, starg.srctname, starg.where);
  stmtsel.bindout(1, tmpvalue, 50);

  // Construct the string for binding the 'where key in (...)' clause for delete SQL statement.
  char bindstr[2001];
  char strtemp[11];

  memset(bindstr, 0, sizeof(bindstr));

  for (int ii = 0; ii < starg.maxcount; ii++)
  {
    memset(strtemp, 0, sizeof(strtemp));
    sprintf(strtemp, ":%lu,", ii + 1);
    strcat(bindstr, strtemp);
  }

  bindstr[strlen(bindstr) - 1] = 0;    // The last comma is redundant.

  char keyvalues[starg.maxcount][51];   // Store the values of the unique key field.

  // Prepare the insert and delete SQL for migrating table data, migrate starg.maxcount records at a time.
  sqlstatement stmtins(&conn2);
  stmtins.prepare("insert into %s(%s) select %s from %s where %s in (%s)", starg.dsttname, TABCOLS.m_allcols, TABCOLS.m_allcols, starg.srctname, starg.keycol, bindstr);

  sqlstatement stmtdel(&conn2);
  stmtdel.prepare("delete from %s where %s in (%s)", starg.srctname, starg.keycol, bindstr);

  for (int ii = 0; ii < starg.maxcount; ii++)
  {
    stmtins.bindin(ii + 1, keyvalues[ii], 50);
    stmtdel.bindin(ii + 1, keyvalues[ii], 50);
  }

  int ccount = 0;
  memset(keyvalues, 0, sizeof(keyvalues));

  if (stmtsel.execute() != 0)
  {
    logfile.Write("stmtsel.execute() failed.\n%s\n%s\n", stmtsel.m_sql, stmtsel.m_cda.message);
    return false;
  }

  while (true)
  {
    memset(tmpvalue, 0, sizeof(tmpvalue));

    // Fetch the result set.
    if (stmtsel.next() != 0)
      break;

    strcpy(keyvalues[ccount], tmpvalue);
    ccount++;

    // Execute migration statements every starg.maxcount records.
    if (ccount == starg.maxcount)
    {
      // Insert into starg.dsttname table first.
      if (stmtins.execute() != 0)
      {
        logfile.Write("stmtins.execute() failed.\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);
        if (stmtins.m_cda.rc != 1062)
          return false;
      }

      // Then delete from starg.srctname table.
      if (stmtdel.execute() != 0)
      {
        logfile.Write("stmtdel.execute() failed.\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message);
        return false;
      }

      conn2.commit();

      ccount = 0;
      memset(keyvalues, 0, sizeof(keyvalues));

      PActive.UptATime();
    }
  }

  // If the number of records is less than starg.maxcount, execute migration again.
  if (ccount > 0)
  {
    if (stmtins.execute() != 0)
    {
      logfile.Write("stmtins.execute() failed.\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);
      if (stmtins.m_cda.rc != 1062)
        return false;
    }

    if (stmtdel.execute() != 0)
    {
      logfile.Write("stmtdel.execute() failed.\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message);
      return false;
    }

    conn2.commit();
  }

  if (stmtsel.m_cda.rpc > 0)
    logfile.Write("Migrated %s to %s %d rows in %.02f sec.\n", starg.srctname, starg.dsttname, stmtsel.m_cda.rpc, Timer.Elapsed());

  return true;
}

// Check if the current time is within the program running time interval.
bool instarttime()
{
  // Program running time interval, e.g., 02,13 means the program runs at 02:00 and 13:00, and does not run at other times.
  if (strlen(starg.starttime) != 0)
  {
    char strHH24[3];
    memset(strHH24, 0, sizeof(strHH24));
    LocalTime(strHH24, "hh24");  // Get only the hour of the current time.
    if (strstr(starg.starttime, strHH24) == 0)
      return false;
  }

  return true;
}

