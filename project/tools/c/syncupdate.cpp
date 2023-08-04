#include "_tools.h"

struct st_arg
{
  // Structure to store various configuration parameters
  // ...
} starg;

CLogFile logfile;
connection connloc; // Local database connection.
connection connrem; // Remote database connection.

// Function to show program help
void _help(char *argv[]);

// Function to parse XML and populate the starg structure with parameters
bool _xmltoarg(char *strxmlbuffer);

// Main business processing function.
bool _syncupdate();

// Function to handle program termination signals
void EXIT(int sig);

CPActive PActive;

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    _help(argv);
    return -1;
  }

  // Close all signals and I/O, handle program termination signals.
  CloseIOAndSignal();
  signal(SIGINT, EXIT);
  signal(SIGTERM, EXIT);

  if (logfile.Open(argv[1], "a+") == false)
  {
    printf("Failed to open the log file (%s).\n", argv[1]);
    return -1;
  }

  // Parse XML and populate the starg structure with parameters
  if (_xmltoarg(argv[2]) == false)
    return -1;

  PActive.AddPInfo(starg.timeout, starg.pname);
  // Note: In the debugging phase, you can enable similar code below to prevent timeouts.
  // PActive.AddPInfo(starg.timeout*100, starg.pname);

  if (connloc.connecttodb(starg.localconnstr, starg.charset) != 0)
  {
    logfile.Write("Failed to connect to the database (%s).\n%s\n", starg.localconnstr, connloc.m_cda.message);
    EXIT(-1);
  }

  // If starg.remotecols or starg.localcols is empty, use all columns of the local table starg.localtname to fill them.
  // ...

  // Main business processing function.
  _syncupdate();
}

// Function to show program help
void _help(char *argv[])
{
  // Function to show help
  // ...
}

// Function to parse XML and populate the starg structure with parameters
bool _xmltoarg(char *strxmlbuffer)
{
  // Parse XML and populate starg structure with parameters
  // ...
  return true;
}

// Main business processing function.
bool _syncupdate()
{
  // Main business processing function.
  // ...
  return true;
}

void EXIT(int sig)
{
  logfile.Write("Program exit, sig=%d\n\n", sig);

  connloc.disconnect();

  connrem.disconnect();

  exit(0);
}




// Main business processing function.
bool _syncupdate()
{
  CTimer Timer;

  sqlstatement stmtdel(&connloc); // SQL statement to delete records from the local table.
  sqlstatement stmtins(&connloc); // SQL statement to insert data into the local table.

  // If it's non-batch synchronization, it means that the amount of data to be synchronized is relatively small, and executing a single SQL statement can handle it.
  if (starg.synctype == 1)
  {
    logfile.Write("Sync %s to %s ...", starg.fedtname, starg.localtname);

    // Delete records from the starg.localtname table that satisfy the where condition.
    stmtdel.prepare("delete from %s %s", starg.localtname, starg.where);
    if (stmtdel.execute() != 0)
    {
      logfile.Write("stmtdel.execute() failed.\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message);
      return false;
    }

    // Insert records from starg.fedtname table that satisfy the where condition into the starg.localtname table.
    stmtins.prepare("insert into %s(%s) select %s from %s %s", starg.localtname, starg.localcols, starg.remotecols, starg.fedtname, starg.where);
    if (stmtins.execute() != 0)
    {
      logfile.Write("stmtins.execute() failed.\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);
      connloc.rollback(); // If this fails, you don't need to rollback the transaction, the destructor of the connection class will handle it.
      return false;
    }

    logfile.WriteEx(" %d rows in %.2fsec.\n", stmtins.m_cda.rpc, Timer.Elapsed());

    connloc.commit();

    return true;
  }

  // Connect to the remote database if synctype==2 (batch synchronization), because if synctype==1, this code is not needed.
  if (connrem.connecttodb(starg.remoteconnstr, starg.charset) != 0)
  {
    logfile.Write("connect database(%s) failed.\n%s\n", starg.remoteconnstr, connrem.m_cda.message);
    return false;
  }

  // ... (some code related to preparing and binding SQL statements)

  int ccount = 0; // Counter for the number of records retrieved from the result set.

  memset(keyvalues, 0, sizeof(keyvalues));

  if (stmtsel.execute() != 0)
  {
    logfile.Write("stmtsel.execute() failed.\n%s\n%s\n", stmtsel.m_sql, stmtsel.m_cda.message);
    return false;
  }

  while (true)
  {
    // Retrieve records from the result set for synchronization.
    if (stmtsel.next() != 0)
      break;

    strcpy(keyvalues[ccount], remkeyvalue);

    ccount++;

    // If ccount reaches starg.maxcount, execute the synchronization in batches.
    if (ccount == starg.maxcount)
    {
      // Delete records from the local table.
      if (stmtdel.execute() != 0)
      {
        // Execution of the delete operation from the local table usually does not fail.
        // If an error occurs, it is likely due to a database problem or incorrect synchronization parameter configuration, and the process does not need to continue.
        logfile.Write("stmtdel.execute() failed.\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message);
        return false;
      }

      // Insert records into the local table.
      if (stmtins.execute() != 0)
      {
        // Execution of the insert operation into the local table usually does not fail.
        // If an error occurs, it is likely due to a database problem or incorrect synchronization parameter configuration, and the process does not need to continue.
        logfile.Write("stmtins.execute() failed.\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);
        return false;
      }

      logfile.Write("Sync %s to %s (%d rows) in %.2fsec.\n", starg.fedtname, starg.localtname, ccount, Timer.Elapsed());

      connloc.commit();

      ccount = 0; // Reset the counter.
      memset(keyvalues, 0, sizeof(keyvalues));

      PActive.UptATime();
    }
  }

  // If ccount > 0, there are remaining records to be synchronized, perform the synchronization one more time.
  if (ccount > 0)
  {
    // Delete records from the local table.
    if (stmtdel.execute() != 0)
    {
      logfile.Write("stmtdel.execute() failed.\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message);
      return false;
    }

    // Insert records into the local table.
    if (stmtins.execute() != 0)
    {
      logfile.Write("stmtins.execute() failed.\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);
      return false;
    }

    logfile.Write("Sync %s to %s (%d rows) in %.2fsec.\n", starg.fedtname, starg.localtname, ccount, Timer.Elapsed());

    connloc.commit();
  }

  return true;
}










