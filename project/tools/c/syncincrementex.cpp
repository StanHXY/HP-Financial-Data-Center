/*
 *  Program Name: syncincrementex.cpp
 *  Description: This program is a common module for the data center, which synchronizes tables between MySQL databases using an incremental approach.
 *  Note: This program does not use the Federated engine.
 */
#include "_tools.h"

struct st_arg
{
  char localconnstr[101];  // Connection parameters for the local database.
  char charset[51];        // Database character set.
  char localtname[31];     // Local table name.
  char remotecols[1001];   // Field list of the remote table.
  char localcols[1001];    // Field list of the local table.
  char where[1001];        // Synchronization data condition.
  char remoteconnstr[101]; // Connection parameters for the remote database.
  char remotetname[31];    // Remote table name.
  char remotekeycol[31];   // Auto-increment field name of the remote table.
  char localkeycol[31];    // Auto-increment field name of the local table.
  int  timetvl;            // Synchronization time interval in seconds, ranging from 1 to 30.
  int  timeout;            // Timeout for the program in seconds, depending on the data volume, it is recommended to set it to 30 or more.
  char pname[51];          // Program name when running.
} starg;

// Display program help.
void _help(char *argv[]);

// Parse xml into the starg structure.
bool _xmltoarg(char *strxmlbuffer);

CLogFile logfile;

connection connloc;   // Connection to the local database.
connection connrem;   // Connection to the remote database.

CTABCOLS TABCOLS;     // Read data dictionary to get all column information of the local table.

// Business processing main function.
bool _syncincrementex(bool &bcontinue);

// Get the maximum value of the auto-increment field from the local table starg.localtname and store it in the global variable maxkeyvalue.
long maxkeyvalue=0;
bool findmaxkey();

void EXIT(int sig);

CPActive PActive;

int main(int argc,char *argv[])
{
  if (argc!=3) { _help(argv); return -1; }

  // Close all signals and input/output, handle program exit signals.
  CloseIOAndSignal(); signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

  if (logfile.Open(argv[1],"a+")==false)
  {
    printf("Failed to open log file (%s).\n",argv[1]); return -1;
  }

  // Parse xml into the starg structure.
  if (_xmltoarg(argv[2])==false) return -1;

  PActive.AddPInfo(starg.timeout,starg.pname);
  // Note: During program debugging, you can enable similar code below to prevent timeouts.
  // PActive.AddPInfo(starg.timeout*100,starg.pname);

  if (connloc.connecttodb(starg.localconnstr,starg.charset) != 0)
  {
    logfile.Write("Connect to database(%s) failed.\n%s\n",starg.localconnstr,connloc.m_cda.message); EXIT(-1);
  }

  // logfile.Write("Connect to database(%s) ok.\n",starg.localconnstr);

  if (connrem.connecttodb(starg.remoteconnstr,starg.charset) != 0)
  {
    logfile.Write("Connect to database(%s) failed.\n%s\n",starg.remoteconnstr,connrem.m_cda.message); return false;
  }

  // logfile.Write("Connect to database(%s) ok.\n",starg.remoteconnstr);

  // Get all columns of the starg.localtname table.
  if (TABCOLS.allcols(&connloc,starg.localtname)==false)
  {
    logfile.Write("Table %s does not exist.\n",starg.localtname); EXIT(-1); 
  }

  if (strlen(starg.remotecols)==0)  strcpy(starg.remotecols,TABCOLS.m_allcols);
  if (strlen(starg.localcols)==0)   strcpy(starg.localcols,TABCOLS.m_allcols);

  bool bcontinue;

  // Business processing main function.
  while (true)
  {
    if (_syncincrementex(bcontinue)==false) EXIT(-1);

    if (bcontinue==false) sleep(starg.timetvl);

    PActive.UptATime();
  }
}

// Display program help.
void _help(char *argv[])
{
  printf("Using:/project/tools1/bin/syncincrementex logfilename xmlbuffer\n\n");

  printf("Sample:/project/tools1/bin/procctl 10 /project/tools1/bin/syncincrementex /log/idc/syncincrementex_ZHOBTMIND2.log \"<localconnstr>192.168.174.129,root,mysqlpwd,mysql,3306</localconnstr><remoteconnstr>192.168.174.132,root,mysqlpwd,mysql,3306</remoteconnstr><charset>utf8</charset><remotetname>T_ZHOBTMIND1</remotetname><localtname>T_ZHOBTMIND2</localtname><remotecols>obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid</remotecols><localcols>stid,ddatetime,t,p,u,wd,wf,r,vis,upttime,recid</localcols><remotekeycol>keyid</remotekeycol><localkeycol>recid</localkeycol><timetvl>2</timetvl><timeout>50</timeout><pname>syncincrementex_ZHOBTMIND2</pname>\"\n\n");

  printf("       /project/tools1/bin/procctl 10 /project/tools1/bin/syncincrementex /log/idc/syncincrementex_ZHOBTMIND3.log \"<localconnstr>192.168.174.129,root,mysqlpwd,mysql,3306</localconnstr><remoteconnstr>192.168.174.132,root,mysqlpwd,mysql,3306</remoteconnstr><charset>utf8</charset><remotetname>T_ZHOBTMIND1</remotetname><localtname>T_ZHOBTMIND3</localtname><remotecols>obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid</remotecols><localcols>stid,ddatetime,t,p,u,wd,wf,r,vis,upttime,recid</localcols><where>and obtid like '54%%%%'</where><remotekeycol>keyid</remotekeycol><localkeycol>recid</localkeycol><timetvl>2</timetvl><timeout>50</timeout><pname>syncincrementex_ZHOBTMIND3</pname>\"\n\n");

  printf("This program is a common module for the data center, which synchronizes tables between MySQL databases using an incremental approach.\n\n");

  printf("logfilename   The log file for program execution.\n");
  printf("xmlbuffer     Program parameters in XML format, as follows:\n\n");

  printf("localconnstr  Connection parameters for the local database in the format: ip,username,password,dbname,port.\n");
  printf("charset       Database character set, this parameter must be consistent with the remote database, otherwise there may be Chinese garbled characters.\n");

  printf("localtname    Local table name.\n");

  printf("remotecols    Field list of the remote table, to be placed between SELECT and FROM, so remotecols can be real fields, function returns, or operation results. If this parameter is empty, it will be filled with the field list of localtname table.\n");
  printf("localcols     Field list of the local table, different from remotecols, it must be real existing fields. If this parameter is empty, it will be filled with the field list of localtname table.\n");

  printf("where         Synchronization data condition, to be placed after SELECT remotekeycol FROM remotetname WHERE remotekeycol>:1, note that do not add the WHERE keyword, but need to add the AND keyword.\n");

  printf("remoteconnstr Connection parameters for the remote database, in the same format as localconnstr.\n");
  printf("remotetname   Remote table name.\n");
  printf("remotekeycol  Auto-increment field name of the remote table.\n");
  printf("localkeycol   Auto-increment field name of the local table.\n");

  printf("timetvl       Synchronization time interval in seconds, ranging from 1 to 30.\n");
  printf("timeout       Timeout for the program in seconds, depending on the data volume, it is recommended to set it to 30 or more.\n");
  printf("pname         Program name when running, try to use an easily understandable and unique name compared to other processes for easy troubleshooting.\n\n\n");
}

// Parse xml into the starg structure.
bool _xmltoarg(char *strxmlbuffer)
{
  memset(&starg,0,sizeof(struct st_arg));

  // Connection parameters for the local database in the format: ip,username,password,dbname,port.
  GetXMLBuffer(strxmlbuffer,"localconnstr",starg.localconnstr,100);
  if (strlen(starg.localconnstr)==0) { logfile.Write("localconnstr is null.\n"); return false; }

  // Database character set, this parameter must be consistent with the remote database, otherwise there may be Chinese garbled characters.
  GetXMLBuffer(strxmlbuffer,"charset",starg.charset,50);
  if (strlen(starg.charset)==0) { logfile.Write("charset is null.\n"); return false; }

  // Local table name.
  GetXMLBuffer(strxmlbuffer,"localtname",starg.localtname,30);
  if (strlen(starg.localtname)==0) { logfile.Write("localtname is null.\n"); return false; }

  // Field list of the remote table, to be placed between SELECT and FROM, so remotecols can be real fields, function returns, or operation results. If this parameter is empty, it will be filled with the field list of localtname table.
  GetXMLBuffer(strxmlbuffer,"remotecols",starg.remotecols,1000);

  // Field list of the local table, different from remotecols, it must be real existing fields. If this parameter is empty, it will be filled with the field list of localtname table.
  GetXMLBuffer(strxmlbuffer,"localcols",starg.localcols,1000);

  // Synchronization data condition, to be placed after SELECT remotekeycol FROM remotetname WHERE remotekeycol>:1, note that do not add the WHERE keyword, but need to add the AND keyword.
  GetXMLBuffer(strxmlbuffer,"where",starg.where,1000);

  // Connection parameters for the remote database, in the same format as localconnstr, valid when synctype==2.
  GetXMLBuffer(strxmlbuffer,"remoteconnstr",starg.remoteconnstr,100);
  if (strlen(starg.remoteconnstr)==0) { logfile.Write("remoteconnstr is null.\n"); return false; }

  // Remote table name, valid when synctype==2.
  GetXMLBuffer(strxmlbuffer,"remotetname",starg.remotetname,30);
  if (strlen(starg.remotetname)==0) { logfile.Write("remotetname is null.\n"); return false; }

  // Auto-increment field name of the remote table.
  GetXMLBuffer(strxmlbuffer,"remotekeycol",starg.remotekeycol,30);
  if (strlen(starg.remotekeycol)==0) { logfile.Write("remotekeycol is null.\n"); return false; }

  // Auto-increment field name of the local table.
  GetXMLBuffer(strxmlbuffer,"localkeycol",starg.localkeycol,30);
  if (strlen(starg.localkeycol)==0) { logfile.Write("localkeycol is null.\n"); return false; }

  // Synchronization time interval in seconds, ranging from 1 to 30.
  GetXMLBuffer(strxmlbuffer,"timetvl",&starg.timetvl);
  if (starg.timetvl<=0) { logfile.Write("timetvl is null.\n"); return false; }
  if (starg.timetvl>30) starg.timetvl=30;

  // Timeout for the program in seconds, depending on the data volume, it is recommended to set it to 30 or more.
  GetXMLBuffer(strxmlbuffer,"timeout",&starg.timeout);
  if (starg.timeout==0) { logfile.Write("timeout is null.\n"); return false; }

  // The following method of handling timetvl and timeout is somewhat arbitrary, but there should be no problem as long as the program does not time out.
  if (starg.timeout<starg.timetvl+10) starg.timeout=starg.timetvl+10;

  // Program name when running, try to use an easily understandable and unique name compared to other processes for easy troubleshooting.
  GetXMLBuffer(strxmlbuffer,"pname",starg.pname,50);
  if (strlen(starg.pname)==0) { logfile.Write("pname is null.\n"); return false; }

  return true;
}


// Business processing main function.
bool _syncincrementex(bool &bcontinue)
{
  CTimer Timer;

  bcontinue = false;

  // Get the maximum value of the auto-increment field from the local table starg.localtname and store it in the global variable maxkeyvalue.
  if (findmaxkey() == false)
    return false;

  // Split the starg.localcols parameter to get the number of fields in the local table.
  CCmdStr CmdStr;
  CmdStr.SplitToCmd(starg.localcols, ",");
  int colcount = CmdStr.CmdCount();

  // Find records from the remote table where the auto-increment field value is greater than maxkeyvalue and store the data in the colvalues array.
  char colvalues[colcount][TABCOLS.m_maxcollen + 1];
  sqlstatement stmtsel(&connrem);
  stmtsel.prepare("select %s from %s where %s>:1 %s order by %s", starg.remotecols, starg.remotetname, starg.remotekeycol, starg.where, starg.remotekeycol);
  stmtsel.bindin(1, &maxkeyvalue);
  for (int ii = 0; ii < colcount; ii++)
    stmtsel.bindout(ii + 1, colvalues[ii], TABCOLS.m_maxcollen);

  // Prepare the SQL statement to insert data into the local table.
  char bindstr[2001]; // String for binding parameters in the synchronization SQL statement.
  char strtemp[11];

  memset(bindstr, 0, sizeof(bindstr));

  for (int ii = 0; ii < colcount; ii++)
  {
    memset(strtemp, 0, sizeof(strtemp));
    sprintf(strtemp, ":%lu,", ii + 1); // This part can handle time fields.
    strcat(bindstr, strtemp);
  }

  bindstr[strlen(bindstr) - 1] = 0; // Remove the last comma.

  sqlstatement stmtins(&connloc); // SQL statement to insert data into the local table.
  stmtins.prepare("insert into %s(%s) values(%s)", starg.localtname, starg.localcols, bindstr);
  for (int ii = 0; ii < colcount; ii++)
  {
    stmtins.bindin(ii + 1, colvalues[ii], TABCOLS.m_maxcollen);
  }

  if (stmtsel.execute() != 0)
  {
    logfile.Write("stmtsel.execute() failed.\n%s\n%s\n", stmtsel.m_sql, stmtsel.m_cda.message);
    return false;
  }

  while (true)
  {
    memset(colvalues, 0, sizeof(colvalues));

    // Get the result set of data to be synchronized.
    if (stmtsel.next() != 0)
      break;

    // Insert the record into the local table.
    if (stmtins.execute() != 0)
    {
      // The operation of inserting records into the local table should generally not fail.
      // If an error occurs, it is likely to be a database problem or incorrect synchronization parameters, and the process does not need to continue.
      logfile.Write("stmtins.execute() failed.\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);
      return false;
    }

    // Commit every 1000 records.
    if (stmtsel.m_cda.rpc % 1000 == 0)
    {
      connloc.commit();
      PActive.UptATime();
    }
  }

  // Process any remaining data that has not been committed.
  if (stmtsel.m_cda.rpc > 0)
  {
    logfile.Write("Sync %s to %s (%d rows) in %.2fsec.\n", starg.remotetname, starg.localtname, stmtsel.m_cda.rpc, Timer.Elapsed());

    connloc.commit();

    bcontinue = true;
  }

  return true;
}

// Get the maximum value of the auto-increment field from the local table starg.localtname and store it in the global variable maxkeyvalue.
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

  // logfile.Write("maxkeyvalue=%ld\n", maxkeyvalue);

  return true;
}









