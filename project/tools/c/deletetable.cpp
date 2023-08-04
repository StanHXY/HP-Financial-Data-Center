#include "_public.h"
#include "_mysql.h"

struct st_arg
{
  char connstr[101];     // Database connection parameters.
  char tname[31];        // Table name to be cleaned.
  char keycol[31];       // Unique key field name of the table to be cleaned.
  char where[1001];      // Conditions that the data to be cleaned must meet.
  char starttime[31];    // Time interval for program execution.
  int  timeout;          // Timeout for the program.
  char pname[51];        // Program name.
} starg;

// Display program help.
void _help(char* argv[]);

// Parse XML to starg structure.
bool _xmltoarg(char* strxmlbuffer);

CLogFile logfile;

// Check if the current time is within the program's execution time interval.
bool instarttime();

connection conn1;     // Database connection for executing query SQL statements.
connection conn2;     // Database connection for executing delete SQL statements.

// Business processing main function.
bool _deletetable();

void EXIT(int sig);

CPActive PActive;

int main(int argc, char* argv[])
{
  if (argc != 3)
  {
    _help(argv);
    return -1;
  }

  // Close all signals and input/output.
  // Handle program exit signals.
  CloseIOAndSignal();
  signal(SIGINT, EXIT);
  signal(SIGTERM, EXIT);

  if (logfile.Open(argv[1], "a+") == false)
  {
    printf("Failed to open log file (%s).\n", argv[1]);
    return -1;
  }

  // Parse XML to starg structure.
  if (_xmltoarg(argv[2]) == false)
    return -1;

  // Check if the current time is within the program's execution time interval.
  if (instarttime() == false)
    return 0;

  PActive.AddPInfo(starg.timeout, starg.pname);
  // Note: You can use the following code during debugging to prevent timeout.
  // PActive.AddPInfo(starg.timeout * 100, starg.pname);

  if (conn1.connecttodb(starg.connstr, NULL) != 0)
  {
    logfile.Write("Connect to database (%s) failed.\n%s\n", starg.connstr, conn1.m_cda.message);
    EXIT(-1);
  }

  if (conn2.connecttodb(starg.connstr, NULL, 1) != 0)  // Enable auto-commit.
  {
    logfile.Write("Connect to database (%s) failed.\n%s\n", starg.connstr, conn2.m_cda.message);
    EXIT(-1);
  }

  // logfile.Write("Connect to database (%s) ok.\n", starg.connstr);

  // Business processing main function.
  _deletetable();
}


// Display program help
void _help(char* argv[])
{
  printf("Using:/project/tools1/bin/deletetable logfilename xmlbuffer\n\n");

  printf("Sample:/project/tools1/bin/procctl 3600 /project/tools1/bin/deletetable /log/idc/deletetable_ZHOBTMIND1.log \"<connstr>127.0.0.1,root,mysqlpwd,mysql,3306</connstr><tname>T_ZHOBTMIND1</tname><keycol>keyid</keycol><where>where ddatetime<timestampadd(minute,-120,now())</where><starttime>01,02,03,04,05,13</starttime><timeout>120</timeout><pname>deletetable_ZHOBTMIND1</pname>\"\n\n");

  printf("This program is a common function module for the data center, used to periodically clean up data in a table.\n");

  printf("logfilename The log file for this program to run.\n");
  printf("xmlbuffer   The parameters for this program to run, represented in XML format, as follows:\n\n");

  printf("connstr     Database connection parameters in the format: ip,username,password,dbname,port.\n");
  printf("tname       The table name of the data to be cleaned.\n");
  printf("keycol      The unique key field name of the table to be cleaned.\n");
  printf("starttime   The time interval for program execution, for example, 02,13 means that the program will run at 02:00 and 13:00, and will not run at other times.\n"\
         "            If starttime is empty, this parameter will be ignored, and the data migration will be executed whenever the program is started. To reduce the\n"\
         "            load on the database, data migration is generally performed when the database is least busy.\n");
  printf("where       Conditions that the data to be cleaned must meet, i.e., the 'where' part of the SQL statement.\n");
  printf("timeout     The timeout time for this program, in seconds, it is recommended to set it above 120.\n");
  printf("pname       Process name, try to use a name that is easy to understand and different from other processes for easy troubleshooting.\n\n");
}

// Parse XML to starg structure
bool _xmltoarg(char* strxmlbuffer)
{
  memset(&starg, 0, sizeof(struct st_arg));

  GetXMLBuffer(strxmlbuffer, "connstr", starg.connstr, 100);
  if (strlen(starg.connstr) == 0) { logfile.Write("connstr is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "tname", starg.tname, 30);
  if (strlen(starg.tname) == 0) { logfile.Write("tname is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "keycol", starg.keycol, 30);
  if (strlen(starg.keycol) == 0) { logfile.Write("keycol is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "where", starg.where, 1000);
  if (strlen(starg.where) == 0) { logfile.Write("where is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "starttime", starg.starttime, 30);

  GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);
  if (starg.timeout == 0) { logfile.Write("timeout is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50);
  if (strlen(starg.pname) == 0) { logfile.Write("pname is null.\n"); return false; }

  return true;
}

void EXIT(int sig)
{
  logfile.Write("Program exit, sig=%d\n\n", sig);

  conn1.disconnect();
  conn2.disconnect();

  exit(0);
}

// Business processing main function.
bool _deletetable()
{
  CTimer Timer;

  char tmpvalue[51];    // Store the value of the unique key extracted from the table.

  // Extract the unique key of the records to be deleted from the table.
  sqlstatement stmtsel(&conn1);
  stmtsel.prepare("select %s from %s %s", starg.keycol, starg.tname, starg.where);
  stmtsel.bindout(1, tmpvalue, 50);

  // Concatenate the string for the 'where' clause in the delete SQL statement, e.g., where keycol in (...).
  char bindstr[2001];
  char strtemp[11];

  memset(bindstr, 0, sizeof(bindstr));

  for (int ii = 0; ii < MAXPARAMS; ii++)
  {
    memset(strtemp, 0, sizeof(strtemp));
    sprintf(strtemp, ":%lu,", ii + 1);
    strcat(bindstr, strtemp);
  }

  bindstr[strlen(bindstr) - 1] = 0;    // The last comma is redundant.

  char keyvalues[MAXPARAMS][51];   // Store the values of the unique key fields.

  // Prepare the SQL to delete data, deleting MAXPARAMS records at a time.
  sqlstatement stmtdel(&conn2);
  stmtdel.prepare("delete from %s where %s in (%s)", starg.tname, starg.keycol, bindstr);
  for (int ii = 0; ii < MAXPARAMS; ii++)
    stmtdel.bindin(ii + 1, keyvalues[ii], 50);

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

    // Get the result set.
    if (stmtsel.next() != 0)
      break;

    strcpy(keyvalues[ccount], tmpvalue);
    ccount++;

    // Execute the delete statement every MAXPARAMS records.
    if (ccount == MAXPARAMS)
    {
      if (stmtdel.execute() != 0)
      {
        logfile.Write("stmtdel.execute() failed.\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message);
        return false;
      }

      ccount = 0;
      memset(keyvalues, 0, sizeof(keyvalues));

      PActive.UptATime();
    }
  }

  // If the number of records is less than MAXPARAMS, execute the delete once again.
  if (ccount > 0)
  {
    if (stmtdel.execute() != 0)
    {
      logfile.Write("stmtdel.execute() failed.\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message);
      return false;
    }
  }

  if (stmtsel.m_cda.rpc > 0)
    logfile.Write("delete from %s %d rows in %.02fsec.\n", starg.tname, stmtsel.m_cda.rpc, Timer.Elapsed());

  return true;
}


// Check if the current time is within the program's running time interval.
bool instarttime()
{
  // The program's running time interval, e.g., 02,13 means the program will run at 02:00 and 13:00, and won't run at other times.
  if (strlen(starg.starttime) != 0)
  {
    char strHH24[3];
    memset(strHH24, 0, sizeof(strHH24));
    LocalTime(strHH24, "hh24");  // Get the current hour only.
    if (strstr(starg.starttime, strHH24) == nullptr)
      return false;
  }

  return true;
}

