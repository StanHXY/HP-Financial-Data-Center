/*
 * Program Name: syncincrement.cpp
 * Description: This program is a common module for the data center, used to synchronize tables between MySQL databases using incremental methods.
 */
#include "_tools.h"

struct st_arg
{
  char localconnstr[101];   // Local database connection parameters.
  char charset[51];         // Database character set.
  char fedtname[31];        // Federated table name.
  char localtname[31];      // Local table name.
  char remotecols[1001];    // Remote table field list.
  char localcols[1001];     // Local table field list.
  char where[1001];         // Synchronization data condition.
  char remoteconnstr[101];  // Remote database connection parameters.
  char remotetname[31];     // Remote table name.
  char remotekeycol[31];    // Remote table auto-increment field name.
  char localkeycol[31];     // Local table auto-increment field name.
  int  maxcount;            // Number of records per batch for synchronization, cannot exceed the MAXPARAMS macro (defined in _mysql.h), valid when synctype == 2.
  int  timetvl;             // Synchronization time interval in seconds, ranging from 1 to 30.
  int  timeout;             // Timeout for the program to run in seconds, depending on the data size, it is recommended to set it to 30 or above.
  char pname[51];           // Program name during runtime, preferably a recognizable and distinct name from other processes to facilitate troubleshooting.
} starg;

// Display program help.
void _help(char *argv[]);

// Parse XML into the starg structure as parameters.
bool _xmltoarg(char *strxmlbuffer);

CLogFile logfile;

connection connloc;   // Local database connection.
connection connrem;   // Remote database connection.

// Main function for business processing.
bool _syncincrement(bool &bcontinue);

// Get the maximum value of the auto-increment field from the local table starg.localtname, stored in the global variable maxkeyvalue.
long maxkeyvalue=0;
bool findmaxkey();

void EXIT(int sig);

CPActive PActive;

int main(int argc, char *argv[])
{
  if (argc != 3) { _help(argv); return -1; }

  // Close all signals and input/output, handle program exit signals.
  CloseIOAndSignal(); signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

  if (logfile.Open(argv[1], "a+") == false)
  {
    printf("Failed to open the log file (%s).\n", argv[1]); return -1;
  }

  // Parse XML into the starg structure as parameters.
  if (_xmltoarg(argv[2]) == false) return -1;

  PActive.AddPInfo(starg.timeout, starg.pname);
  // Note: When debugging the program, you can enable similar code below to prevent timeouts.
  // PActive.AddPInfo(starg.timeout * 100, starg.pname);

  if (connloc.connecttodb(starg.localconnstr, starg.charset) != 0)
  {
    logfile.Write("Failed to connect to the database (%s).\n%s\n", starg.localconnstr, connloc.m_cda.message); EXIT(-1);
  }

  // logfile.Write("Connected to the database (%s) successfully.\n", starg.localconnstr);

  if (connrem.connecttodb(starg.remoteconnstr, starg.charset) != 0)
  {
    logfile.Write("Failed to connect to the database (%s).\n%s\n", starg.remoteconnstr, connrem.m_cda.message); return false;
  }

  // logfile.Write("Connected to the database (%s) successfully.\n", starg.remoteconnstr);

  // If starg.remotecols or starg.localcols is empty, use all columns of the starg.localtname table to fill them.
  if ((strlen(starg.remotecols) == 0) || (strlen(starg.localcols) == 0))
  {
    CTABCOLS TABCOLS;

    // Get all columns of the starg.localtname table.
    if (TABCOLS.allcols(&connloc, starg.localtname) == false)
    {
      logfile.Write("Table %s does not exist.\n", starg.localtname); EXIT(-1);
    }

    if (strlen(starg.remotecols) == 0)  strcpy(starg.remotecols, TABCOLS.m_allcols);
    if (strlen(starg.localcols) == 0)   strcpy(starg.localcols, TABCOLS.m_allcols);
  }

  bool bcontinue;

  // Main function for business processing.
  while (true)
  {
    if (_syncincrement(bcontinue) == false) EXIT(-1);

    if (bcontinue == false) sleep(starg.timetvl);

    PActive.UptATime();
  }
}

// Display program help.
void _help(char *argv[])
{
  printf("Using: /project/tools1/bin/syncincrement logfilename xmlbuffer\n\n");

  printf("Sample: /project/tools1/bin/procctl 10 /project/tools1/bin/syncincrement /log/idc/syncincrement_ZHOBTMIND2.log \"<localconnstr>192.168.174.129,root,mysqlpwd,mysql,3306</localconnstr><remoteconnstr>192.168.174.132,root,mysqlpwd,mysql,3306</remoteconnstr><charset>utf8</charset><remotetname>T_ZHOBTMIND1</remotetname><fedtname>LK_ZHOBTMIND1</fedtname><localtname>T_ZHOBTMIND2</localtname><remotecols>obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid</remotecols><localcols>stid,ddatetime,t,p,u,wd,wf,r,vis,upttime,recid</localcols><remotekeycol>keyid</remotekeycol><localkeycol>recid</localkeycol><maxcount>300</maxcount><timetvl>2</timetvl><timeout>50</timeout><pname>syncincrement_ZHOBTMIND2</pname>\"\n\n");

  printf("       /project/tools1/bin/procctl 10 /project/tools1/bin/syncincrement /log/idc/syncincrement_ZHOBTMIND3.log \"<localconnstr>192.168.174.129,root,mysqlpwd,mysql,3306</localconnstr><remoteconnstr>192.168.174.132,root,mysqlpwd,mysql,3306</remoteconnstr><charset>utf8</charset><remotetname>T_ZHOBTMIND1</remotetname><fedtname>LK_ZHOBTMIND1</fedtname><localtname>T_ZHOBTMIND3</localtname><remotecols>obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid</remotecols><localcols>stid,ddatetime,t,p,u,wd,wf,r,vis,upttime,recid</localcols><where>and obtid like '54%%%%'</where><remotekeycol>keyid</remotekeycol><localkeycol>recid</localkeycol><maxcount>300</maxcount><timetvl>2</timetvl><timeout>50</timeout><pname>syncincrement_ZHOBTMIND3</pname>\"\n\n");

  printf("This program is a common module for the data center, used to synchronize tables between MySQL databases using incremental methods.\n\n");

  printf("logfilename   The log file for program runtime.\n");
  printf("xmlbuffer     Program parameters represented in XML format, as follows:\n\n");

  printf("localconnstr  Local database connection parameters in the format: ip,username,password,dbname,port.\n");
  printf("charset       Database character set, which must be consistent with the remote database, otherwise Chinese garbled characters may occur.\n");

  printf("fedtname      Federated table name.\n");
  printf("localtname    Local table name.\n");

  printf("remotecols    Remote table field list, used to fill between select and from, so remotecols can be real fields, function returns, or operation results. If this parameter is empty, the fields list of localtname table will be used for filling.\n");
  printf("localcols     Local table field list, unlike remotecols, it must be real existing fields. If this parameter is empty, the fields list of localtname table will be used for filling.\n");

  printf("where         Synchronization data condition, to be filled in after select remotekeycol from remotetname where remotekeycol >:1, note that you should not add the 'where' keyword, but add the 'and' keyword.\n");

  printf("remoteconnstr Remote database connection parameters in the same format as localconnstr.\n");
  printf("remotetname   Remote table name.\n");
  printf("remotekeycol  Remote table auto-increment field name.\n");
  printf("localkeycol   Local table auto-increment field name.\n");

  printf("maxcount      Number of records per batch for synchronization, cannot exceed the MAXPARAMS macro (defined in _mysql.h), valid when synctype == 2.\n");

  printf("timetvl       Synchronization time interval in seconds, ranging from 1 to 30.\n");
  printf("timeout       Timeout for the program to run in seconds, depending on the data size, it is recommended to set it to 30 or above.\n");
  printf("pname         Program name during runtime, preferably a recognizable and distinct name from other processes to facilitate troubleshooting.\n\n\n");
}

// Parse XML into the starg structure as parameters.
bool _xmltoarg(char *strxmlbuffer)
{
  memset(&starg, 0, sizeof(struct st_arg));

  // Local database connection parameters in the format: ip,username,password,dbname,port.
  GetXMLBuffer(strxmlbuffer, "localconnstr", starg.localconnstr, 100);
  if (strlen(starg.localconnstr) == 0) { logfile.Write("localconnstr is null.\n"); return false; }

  // Database character set, which must be consistent with the remote database, otherwise Chinese garbled characters may occur.
  GetXMLBuffer(strxmlbuffer, "charset", starg.charset, 50);
  if (strlen(starg.charset) == 0) { logfile.Write("charset is null.\n"); return false; }

  // Federated table name.
  GetXMLBuffer(strxmlbuffer, "fedtname", starg.fedtname, 30);
  if (strlen(starg.fedtname) == 0) { logfile.Write("fedtname is null.\n"); return false; }

  // Local table name.
  GetXMLBuffer(strxmlbuffer, "localtname", starg.localtname, 30);
  if (strlen(starg.localtname) == 0) { logfile.Write("localtname is null.\n"); return false; }

  // Remote table field list, used to fill between select and from, so remotecols can be real fields, function returns, or operation results. If this parameter is empty, the fields list of localtname table will be used for filling.
  GetXMLBuffer(strxmlbuffer, "remotecols", starg.remotecols, 1000);

  // Local table field list, unlike remotecols, it must be real existing fields. If this parameter is empty, the fields list of localtname table will be used for filling.
  GetXMLBuffer(strxmlbuffer, "localcols", starg.localcols, 1000);

  // Synchronization data condition, to be filled in after select remotekeycol from remotetname where remotekeycol >:1, note that you should not add the 'where' keyword, but add the 'and' keyword.
  GetXMLBuffer(strxmlbuffer, "where", starg.where, 1000);

  // Remote database connection parameters in the same format as localconnstr.
  GetXMLBuffer(strxmlbuffer, "remoteconnstr", starg.remoteconnstr, 100);
  if (strlen(starg.remoteconnstr) == 0) { logfile.Write("remoteconnstr is null.\n"); return false; }

  // Remote table name, valid when synctype == 2.
  GetXMLBuffer(strxmlbuffer, "remotetname", starg.remotetname, 30);
  if (strlen(starg.remotetname) == 0) { logfile.Write("remotetname is null.\n"); return false; }

  // Remote table auto-increment field name.
  GetXMLBuffer(strxmlbuffer, "remotekeycol", starg.remotekeycol, 30);
  if (strlen(starg.remotekeycol) == 0) { logfile.Write("remotekeycol is null.\n"); return false; }

  // Local table auto-increment field name.
  GetXMLBuffer(strxmlbuffer, "localkeycol", starg.localkeycol, 30);
  if (strlen(starg.localkeycol) == 0) { logfile.Write("localkeycol is null.\n"); return false; }

  // Number of records per batch for synchronization, cannot exceed the MAXPARAMS macro (defined in _mysql.h), valid when synctype == 2.
  GetXMLBuffer(strxmlbuffer, "maxcount", &starg.maxcount);
  if (starg.maxcount == 0) { logfile.Write("maxcount is null.\n"); return false; }
  if (starg.maxcount > MAXPARAMS) starg.maxcount = MAXPARAMS;

  // Synchronization time interval in seconds, ranging from 1 to 30.
  GetXMLBuffer(strxmlbuffer, "timetvl", &starg.timetvl);
  if (starg.timetvl <= 0) { logfile.Write("timetvl is null.\n"); return false; }
  if (starg.timetvl > 30) starg.timetvl = 30;

  // Timeout for the program to run in seconds, depending on the data size, it is recommended to set it to 30 or above.
  GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);
  if (starg.timeout == 0) { logfile.Write("timeout is null.\n"); return false; }

  // The following method of handling timetvl and timeout is somewhat arbitrary, but it should be fine as long as the program does not time out.
  if (starg.timeout < starg.timetvl + 10) starg.timeout = starg.timetvl + 10;

  // Program name during runtime, preferably a recognizable and distinct name from other processes to facilitate troubleshooting.
  GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50);
  if (strlen(starg.pname) == 0) { logfile.Write("pname is null.\n"); return false; }

  return true;
}

void EXIT(int sig)
{
  logfile.Write("Program exited, sig=%d\n\n", sig);

  connloc.disconnect();

  connrem.disconnect();

  exit(0);
}
 



// Business processing main function.
bool _syncincrement(bool &bcontinue)
{
  CTimer Timer;

  bcontinue = false;

  // Get the maximum value of the auto-increment field from the local table starg.localtname and store it in the global variable maxkeyvalue.
  if (findmaxkey() == false) return false;

  // Retrieve records from the remote table where the value of the auto-increment field is greater than maxkeyvalue.
  char remkeyvalue[51]; // The value of the key field to be synchronized, obtained from the remote table.
  sqlstatement stmtsel(&connrem);
  stmtsel.prepare("select %s from %s where %s>:1 %s order by %s", starg.remotekeycol, starg.remotetname, starg.remotekeycol, starg.where, starg.remotekeycol);
  stmtsel.bindin(1, &maxkeyvalue);
  stmtsel.bindout(1, remkeyvalue, 50);

  // Concatenate the string for binding parameters of the synchronization SQL statement (:1,:2,:3,...,:starg.maxcount).
  char bindstr[2001]; // String for binding parameters of the synchronization SQL statement.
  char strtemp[11];

  memset(bindstr, 0, sizeof(bindstr));

  for (int ii = 0; ii < starg.maxcount; ii++)
  {
    memset(strtemp, 0, sizeof(strtemp));
    sprintf(strtemp, ":%lu,", ii + 1);
    strcat(bindstr, strtemp);
  }

  bindstr[strlen(bindstr) - 1] = 0; // The last comma is redundant.

  char keyvalues[starg.maxcount][51]; // Store the values of the key field.

  // Prepare SQL statement for inserting data into the local table, inserting starg.maxcount records at a time.
  // insert into T_ZHOBTMIND2(stid ,ddatetime,t,p,u,wd,wf,r,vis,upttime,recid)
  //                   select obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid from LK_ZHOBTMIND1
  //                    where keyid in (:1,:2,:3);
  sqlstatement stmtins(&connloc); // SQL statement for inserting data into the local table.
  stmtins.prepare("insert into %s(%s) select %s from %s where %s in (%s)", starg.localtname, starg.localcols, starg.remotecols, starg.fedtname, starg.remotekeycol, bindstr);
  for (int ii = 0; ii < starg.maxcount; ii++)
  {
    stmtins.bindin(ii + 1, keyvalues[ii], 50);
  }

  int ccount = 0; // Counter for the number of records retrieved from the result set.

  memset(keyvalues, 0, sizeof(keyvalues));

  if (stmtsel.execute() != 0)
  {
    logfile.Write("stmtsel.execute() failed.\n%s\n%s\n", stmtsel.m_sql, stmtsel.m_cda.message);
    return false;
  }

  while (true)
  {
    // Retrieve the result set of data to be synchronized.
    if (stmtsel.next() != 0)
      break;

    strcpy(keyvalues[ccount], remkeyvalue);

    ccount++;

    // Synchronize every starg.maxcount records.
    if (ccount == starg.maxcount)
    {
      // Insert records into the local table.
      if (stmtins.execute() != 0)
      {
        // Executing the operation to insert records into the local table usually does not encounter errors.
        // If there is an error, it must be a problem with the database or incorrect synchronization parameters, and the process does not need to continue.
        logfile.Write("stmtins.execute() failed.\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);
        return false;
      }

      // logfile.Write("sync %s to %s(%d rows) in %.2fsec.\n",starg.fedtname,starg.localtname,ccount,Timer.Elapsed());

      connloc.commit();

      ccount = 0; // Reset the counter for the number of records retrieved from the result set.

      memset(keyvalues, 0, sizeof(keyvalues));

      PActive.UptATime();
    }
  }

  // If ccount > 0, it means there are more records to be synchronized, perform synchronization again.
  if (ccount > 0)
  {
    // Insert records into the local table.
    if (stmtins.execute() != 0)
    {
      logfile.Write("stmtins.execute() failed.\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);
      return false;
    }

    // logfile.Write("sync %s to %s(%d rows) in %.2fsec.\n",starg.fedtname,starg.localtname,ccount,Timer.Elapsed());

    connloc.commit();
  }

  if (stmtsel.m_cda.rpc > 0)
  {
    logfile.Write("sync %s to %s(%d rows) in %.2fsec.\n", starg.fedtname, starg.localtname, stmtsel.m_cda.rpc, Timer.Elapsed());
    bcontinue = true;
  }

  return true;
}

// Get the maximum value of the auto-increment field from the local table starg.localtname and store it in maxkeyvalue global variable.
bool findmaxkey()
{
  maxkeyvalue = 0;

  sqlstatement stmt(&connloc);
  stmt.prepare("select max(%s) from %s", starg.localkeycol, starg.localtname);
  stmt.bindout(1, &maxkeyvalue);

  if (stmt.execute() != 0)
  {
    logfile.Write("stmt.execute() failed.\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message);
    return false;
  }

  stmt.next();

  // logfile.Write("maxkeyvalue=%ld\n",maxkeyvalue);

  return true;
}









