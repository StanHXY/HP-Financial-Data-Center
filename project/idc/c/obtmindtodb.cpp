/*
 * obtmindtodb.cpp, this program is used to insert stock data
 * into the T_ZHOBTMIND table in the database, supporting both xml and csv file formats.
 */
#include "idcapp.h"

CLogFile logfile;

connection conn;

CPActive PActive;

void EXIT(int sig);

// Main function for business processing.
bool _obtmindtodb(char *pathname, char *connstr, char *charset);

int main(int argc, char *argv[])
{
  // Help documentation.
  if (argc != 5)
  {
    printf("\n");
    printf("Usage: ./obtmindtodb pathname connstr charset logfile\n");

    printf("Example: /project/tools1/bin/procctl 10 /project/idc1/bin/obtmindtodb /idcdata/surfdata \"127.0.0.1,root,mysqlpwd,mysql,3306\" utf8 /log/idc/obtmindtodb.log\n\n");

    printf("This program is used to save national station minute observation data into the T_ZHOBTMIND table of the database, only insert, not update.\n");
    printf("pathname: Directory where national station minute observation data files are stored.\n");
    printf("connstr: Database connection parameters: ip,username,password,dbname,port\n");
    printf("charset: Database character set.\n");
    printf("logfile: Log file name for this program's run.\n");
    printf("The program runs every 10 seconds, scheduled by procctl.\n\n\n");

    return -1;
  }

  // Close all signals and I/O.
  // Set signals, in shell mode, "kill + process number" can terminate these processes normally.
  // But please do not use "kill -9 + process number" to forcibly terminate.
  CloseIOAndSignal();
  signal(SIGINT, EXIT);
  signal(SIGTERM, EXIT);

  // Open the log file.
  if (logfile.Open(argv[4], "a+") == false)
  {
    printf("Failed to open the log file (%s).\n", argv[4]);
    return -1;
  }

  // PActive.AddPInfo(30, "obtmindtodb"); // Process heartbeat, 30 seconds is enough.
  // Note: When debugging the program, you can enable code similar to the following to prevent timeouts.
  PActive.AddPInfo(5000, "obtmindtodb");

  // Main function for business processing.
  _obtmindtodb(argv[1], argv[2], argv[3]);

  return 0;
}

void EXIT(int sig)
{
  logfile.Write("Program exits, sig=%d\n\n", sig);

  conn.disconnect();

  exit(0);
}

// Main function for business processing.
bool _obtmindtodb(char *pathname, char *connstr, char *charset)
{
  CDir Dir;

  // Open the directory.
  if (Dir.OpenDir(pathname, "*.xml") == false)
  {
    logfile.Write("Dir.OpenDir(%s) failed.\n", pathname);
    return false;
  }

  CFile File;

  CZHOBTMIND ZHOBTMIND(&conn, &logfile);

  int totalcount = 0;  // Total number of records in the file.
  int insertcount = 0; // Number of successfully inserted records.
  CTimer Timer;       // Timer to record the processing time for each data file.

  while (true)
  {
    // Read the directory and get a data file name.
    if (Dir.ReadDir() == false)
      break;

    // Connect to the database.
    if (conn.m_state == 0)
    {
      if (conn.connecttodb(connstr, charset) != 0)
      {
        logfile.Write("Connect to the database(%s) failed.\n%s\n", connstr, conn.m_cda.message);
        return -1;
      }

      logfile.Write("Connect to the database(%s) ok.\n", connstr);
    }

    totalcount = insertcount = 0;

    // Open the file.
    if (File.Open(Dir.m_FullFileName, "r") == false)
    {
      logfile.Write("File.Open(%s) failed.\n", Dir.m_FullFileName);
      return false;
    }

    char strBuffer[1001];   // Store each line read from the file.

    while (true)
    {
      if (File.FFGETS(strBuffer, 1000, "<endl/>") == false)
        break;

      // Process each line in the file.
      totalcount++;

      ZHOBTMIND.SplitBuffer(strBuffer);

      if (ZHOBTMIND.InsertTable() == true)
        insertcount++;
    }

    // Delete the file and commit the transaction.
    // File.CloseAndRemove();

    conn.commit();

    logfile.Write("Processed file %s (totalcount=%d, insertcount=%d), elapsed time: %.2f seconds.\n", Dir.m_FullFileName, totalcount, insertcount, Timer.Elapsed());
  }

  return true;
}
