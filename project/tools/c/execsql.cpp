/*
 *  execsql.cpp, a utility program for executing an SQL script file.
 *  Author: Wu Congzhou.
 */
#include "_public.h"
#include "_mysql.h"

CLogFile logfile;

connection conn;

CPActive PActive;

void EXIT(int sig);

int main(int argc, char *argv[])
{
  // Help documentation.
  if (argc != 5)
  {
    printf("\n");
    printf("Using: ./execsql sqlfile connstr charset logfile\n");

    printf("Example: /project/tools1/bin/procctl 120 /project/tools1/bin/execsql /project/idc/sql/cleardata.sql \"127.0.0.1,root,mysqlpwd,mysql,3306\" utf8 /log/idc/execsql.log\n\n");

    printf("This is a utility program for executing an SQL script file.\n");
    printf("sqlfile: SQL script file name. Each SQL statement can be written on multiple lines, and a semicolon ';' marks the end of a SQL statement. Comments are not supported.\n");
    printf("connstr: Database connection parameters in the format: ip,username,password,dbname,port.\n");
    printf("charset: Database character set.\n");
    printf("logfile: Log file name for this program.\n\n");

    return -1;
  }

  // Close all signals and input/output.
  // Set the signal so that "kill + process number" can terminate the process normally in the shell.
  // But please do not use "kill -9 + process number" to force termination.
  CloseIOAndSignal();
  signal(SIGINT, EXIT);
  signal(SIGTERM, EXIT);

  // Open the log file.
  if (logfile.Open(argv[4], "a+") == false)
  {
    printf("Failed to open the log file (%s).\n", argv[4]);
    return -1;
  }

  PActive.AddPInfo(500, "obtcodetodb"); // Process heartbeat, long interval.
  // Note that, during debugging, you can use similar code below to prevent timeouts.
  // PActive.AddPInfo(5000, "obtcodetodb");

  // Connect to the database, without enabling transactions.
  if (conn.connecttodb(argv[2], argv[3], 1) != 0)
  {
    logfile.Write("Failed to connect to the database (%s).\n%s\n", argv[2], conn.m_cda.message);
    return -1;
  }

  logfile.Write("Database connected successfully (%s).\n", argv[2]);

  CFile File;

  // Open the SQL file.
  if (File.Open(argv[1], "r") == false)
  {
    logfile.Write("Failed to open the file (%s).\n", argv[1]);
    EXIT(-1);
  }

  char strsql[1001]; // Buffer to store the SQL statement read from the SQL file.

  while (true)
  {
    memset(strsql, 0, sizeof(strsql));

    // Read one line terminated by a semicolon from the SQL file.
    if (File.FFGETS(strsql, 1000, ";") == false)
      break;

    // If the first character is '#', it's a comment, do not execute.
    if (strsql[0] == '#')
      continue;

    // Remove the trailing semicolon from the SQL statement.
    char *pp = strstr(strsql, ";");
    if (pp == 0)
      continue;
    pp[0] = 0;

    logfile.Write("%s\n", strsql);

    int iret = conn.execute(strsql); // Execute the SQL statement.

    // Write the execution result of the SQL statement to the log.
    if (iret == 0)
      logfile.Write("Execution succeeded (rpc=%d).\n", conn.m_cda.rpc);
    else
      logfile.Write("Execution failed (%s).\n", conn.m_cda.message);

    PActive.UptATime(); // Process heartbeat.
  }

  logfile.WriteEx("\n");

  return 0;
}

void EXIT(int sig)
{
  logfile.Write("Program exit, sig=%d\n\n", sig);

  conn.disconnect();

  exit(0);
}
