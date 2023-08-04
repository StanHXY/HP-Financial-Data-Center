/*
 * Program Name: dminingmysql6.cpp
 * Description: This program is a common module in the data center, used for extracting data from a MySQL database source table and generating XML files.
 */
#include "_public.h"
#include "_mysql.h"

// Structure for program runtime parameters.
struct st_arg
{
  char connstr[101];     // Database connection parameters.
  char charset[51];      // Database character set.
  char selectsql[1024];  // SQL statement to extract data from the data source database.
  char fieldstr[501];    // Field names in the SQL statement's output result set, separated by commas.
  char fieldlen[501];    // Lengths of the fields in the SQL statement's output result set, separated by commas.
  char bfilename[31];    // Prefix for the output XML file.
  char efilename[31];    // Suffix for the output XML file.
  char outpath[301];     // Directory to store the output XML files.
  char starttime[52];    // Program's running time interval.
  char incfield[31];     // Incremental field name.
  char incfilename[301]; // File to store the maximum value of the incremental field from the extracted data.
  int  timeout;          // Timeout for process heartbeat.
  char pname[51];        // Process name, suggested to use "dminingmysql_" followed by a suffix.
} starg;

#define MAXFIELDCOUNT  100  // Maximum number of fields in the result set.
//#define MAXFIELDLEN    500  // Maximum length of the field value in the result set.
int MAXFIELDLEN = -1;   // Maximum length of the field value in the result set, storing the maximum value among the elements in the fieldlen array.

char strfieldname[MAXFIELDCOUNT][31]; // Array to store result set field names, parsed from starg.fieldstr.
int  ifieldlen[MAXFIELDCOUNT];        // Array to store result set field lengths, parsed from starg.fieldlen.
int  ifieldcount;                     // Number of valid fields in strfieldname and ifieldlen arrays.
int  incfieldpos = -1;                // Position of the incremental field in the result set array.

connection conn;

CLogFile logfile;

// Function to handle program exit and signals 2, 15.
void EXIT(int sig);

void _help();

// Function to parse XML and extract program runtime parameters.
bool _xmltoarg(char *strxmlbuffer);

// Function to check if the current time is within the program's running time interval.
bool instarttime();

// Main function for data extraction.
bool _dminingmysql();

CPActive PActive; // Process heartbeat.

char strxmlfilename[301]; // XML file name.
void crtxmlfilename();    // Function to generate XML file names.

long imaxincvalue;    // Maximum value of the incremental field.
bool readincfile();   // Function to read the maximum id of the extracted data from starg.incfilename file.
bool writeincfile();  // Function to write the maximum id of the extracted data to starg.incfilename file.

int main(int argc, char *argv[])
{
  if (argc != 3) { _help(); return -1; }

  // Close all signals and input/output.
  // Set signals, in shell mode, "kill + process number" can terminate these processes normally.
  // But please don't use "kill -9 + process number" to forcefully terminate them.
  // CloseIOAndSignal();
  signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

  // Open the log file.
  if (logfile.Open(argv[1], "a+") == false)
  {
    printf("Failed to open the log file (%s).\n", argv[1]); return -1;
  }

  // Parse the XML to get the program's runtime parameters.
  if (_xmltoarg(argv[2]) == false) return -1;

  // Check if the current time is within the program's running time interval.
  if (instarttime() == false) return 0;

  PActive.AddPInfo(starg.timeout, starg.pname); // Write process heartbeat information to shared memory.
  // Note: During debugging, you can enable similar code below to prevent timeouts.
  // PActive.AddPInfo(5000, starg.pname);

  // Connect to the database.
  if (conn.connecttodb(starg.connstr, starg.charset) != 0)
  {
    logfile.Write("Failed to connect to the database (%s).\n%s\n", starg.connstr, conn.m_cda.message); return -1;
  }

  logfile.Write("Connected to the database (%s) successfully.\n", starg.connstr);

  _dminingmysql();

  return 0;
}


// Main function for data extraction.
bool _dminingmysql()
{
  // Get the maximum id of the extracted data from starg.incfilename file.
  readincfile();

  sqlstatement stmt(&conn);
  stmt.prepare(starg.selectsql);
  char strfieldvalue[ifieldcount][MAXFIELDLEN + 1]; // Array to store the result set field values after data extraction.
  for (int ii = 1; ii <= ifieldcount; ii++)
  {
    stmt.bindout(ii, strfieldvalue[ii - 1], ifieldlen[ii - 1]);
  }

  // If it is an incremental extraction, bind the input parameter (the maximum id of the extracted data).
  if (strlen(starg.incfield) != 0) stmt.bindin(1, &imaxincvalue);

  if (stmt.execute() != 0)
  {
    logfile.Write("stmt.execute() failed.\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message);
    return false;
  }

  PActive.UptATime();

  CFile File; // Used to operate on XML files.

  while (true)
  {
    memset(strfieldvalue, 0, sizeof(strfieldvalue));

    if (stmt.next() != 0) break;

    if (File.IsOpened() == false)
    {
      crtxmlfilename(); // Generate XML file name.

      if (File.OpenForRename(strxmlfilename, "w+") == false)
      {
        logfile.Write("File.OpenForRename(%s) failed.\n", strxmlfilename);
        return false;
      }

      File.Fprintf("<data>\n");
    }

    for (int ii = 1; ii <= ifieldcount; ii++)
      File.Fprintf("<%s>%s</%s>", strfieldname[ii - 1], strfieldvalue[ii - 1], strfieldname[ii - 1]);

    File.Fprintf("<endl/>\n");

    // Switch to a new XML file if the number of records reaches 1000 rows.
    if (stmt.m_cda.rpc % 1000 == 0)
    {
      File.Fprintf("</data>\n");

      if (File.CloseAndRename() == false)
      {
        logfile.Write("File.CloseAndRename(%s) failed.\n", strxmlfilename);
        return false;
      }

      logfile.Write("Generated file %s(1000).\n", strxmlfilename);

      PActive.UptATime();
    }

    // Update the maximum value of the incremental field.
    if (imaxincvalue < atol(strfieldvalue[incfieldpos])) imaxincvalue = atol(strfieldvalue[incfieldpos]);
  }

  if (File.IsOpened() == true)
  {
    File.Fprintf("</data>\n");

    if (File.CloseAndRename() == false)
    {
      logfile.Write("File.CloseAndRename(%s) failed.\n", strxmlfilename);
      return false;
    }

    logfile.Write("Generated file %s(%d).\n", strxmlfilename, stmt.m_cda.rpc % 1000);
  }

  // Write the maximum value of the incremental field to starg.incfilename file.
  if (stmt.m_cda.rpc > 0) writeincfile();

  return true;
}


void EXIT(int sig)
{
  printf("Program exit, sig=%d\n\n", sig);
  exit(0);
}

void _help()
{
  printf("Usage: /project/tools1/bin/dminingmysql logfilename xmlbuffer\n\n");

  printf("Sample: /project/tools1/bin/procctl 3600 /project/tools1/bin/dminingmysql /log/idc/dminingmysql_ZHOBTCODE.log \"<connstr>127.0.0.1,root,mysqlpwd,mysql,3306</connstr><charset>utf8</charset><selectsql>select obtid,cityname,provname,lat,lon,height from T_ZHOBTCODE</selectsql><fieldstr>obtid,cityname,provname,lat,lon,height</fieldstr><fieldlen>10,30,30,10,10,10</fieldlen><bfilename>ZHOBTCODE</bfilename><efilename>HYCZ</efilename><outpath>/idcdata/dmindata</outpath><timeout>30</timeout><pname>dminingmysql_ZHOBTCODE</pname>\"\n\n");
  printf("       /project/tools1/bin/procctl 30 /project/tools1/bin/dminingmysql /log/idc/dminingmysql_ZHOBTMIND.log \"<connstr>127.0.0.1,root,mysqlpwd,mysql,3306</connstr><charset>utf8</charset><selectsql>select obtid,date_format(ddatetime,'%%%%Y-%%%%m-%%%%d %%%%H:%%%%i:%%%%s'),t,p,u,wd,wf,r,vis,keyid from T_ZHOBTMIND where keyid>:1 and ddatetime>timestampadd(minute,-120,now())</selectsql><fieldstr>obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid</fieldstr><fieldlen>10,19,8,8,8,8,8,8,8,15</fieldlen><bfilename>ZHOBTMIND</bfilename><efilename>HYCZ</efilename><outpath>/idcdata/dmindata</outpath><starttime></starttime><incfield>keyid</incfield><incfilename>/idcdata/dmining/dminingmysql_ZHOBTMIND_HYCZ.list</incfilename><timeout>30</timeout><pname>dminingmysql_ZHOBTMIND_HYCZ</pname>\"\n\n");

  printf("This program is a common module for the data center. It is used to extract data from a MySQL database source table and generate XML files.\n");
  printf("logfilename The log file for this program run.\n");
  printf("xmlbuffer   The program's parameters in XML format, as follows:\n\n");

  printf("connstr     Database connection parameters in the format: ip,username,password,dbname,port.\n");
  printf("charset     Database character set, this parameter should be consistent with the source database, otherwise, it may cause Chinese garbled characters.\n");
  printf("selectsql   SQL statement to extract data from the source database. Note: The percentage sign '%%' in time functions should be represented by four percentage signs '%%%%'. After 'prepare', it will become two percentage signs '%%'.\n");
  printf("fieldstr    The names of the fields in the output result set of the SQL statement, separated by commas, which will be used as field names in the XML file.\n");
  printf("fieldlen    The lengths of the fields in the output result set of the SQL statement, separated by commas. The fields in 'fieldstr' and 'fieldlen' must correspond one-to-one.\n");
  printf("bfilename   The prefix of the output XML files.\n");
  printf("efilename   The suffix of the output XML files.\n");
  printf("outpath     The directory where the output XML files will be stored.\n");
  printf("starttime   The program's running time interval. For example, '02,13' means that the program will run only if it starts at 02:00 and 13:00. It will not run at other times. If starttime is empty, this parameter will be ignored, and the data extraction will be executed whenever the program starts. To reduce the pressure on the data source, data extraction is usually performed when the source database is least busy.\n");
  printf("incfield    The name of the incremental field. It must be one of the fields in 'fieldstr' and must be an integer, usually an auto-increment field. If incfield is empty, incremental extraction will not be used.\n");
  printf("incfilename The file that stores the maximum value of the incremental field for the extracted data. If this file is lost, all data will be re-extracted.\n");
  printf("timeout     The timeout time for this program in seconds.\n");
  printf("pname       The process name, it is recommended to use an easy-to-understand name different from other processes to facilitate troubleshooting.\n\n\n");
}

// Parse XML and store it into the starg structure.
bool _xmltoarg(char *strxmlbuffer)
{
  memset(&starg, 0, sizeof(struct st_arg));

  GetXMLBuffer(strxmlbuffer, "connstr", starg.connstr, 100);
  if (strlen(starg.connstr) == 0) { logfile.Write("connstr is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "charset", starg.charset, 50);
  if (strlen(starg.charset) == 0) { logfile.Write("charset is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "selectsql", starg.selectsql, 1000);
  if (strlen(starg.selectsql) == 0) { logfile.Write("selectsql is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "fieldstr", starg.fieldstr, 500);
  if (strlen(starg.fieldstr) == 0) { logfile.Write("fieldstr is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "fieldlen", starg.fieldlen, 500);
  if (strlen(starg.fieldlen) == 0) { logfile.Write("fieldlen is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "bfilename", starg.bfilename, 30);
  if (strlen(starg.bfilename) == 0) { logfile.Write("bfilename is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "efilename", starg.efilename, 30);
  if (strlen(starg.efilename) == 0) { logfile.Write("efilename is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "outpath", starg.outpath, 300);
  if (strlen(starg.outpath) == 0) { logfile.Write("outpath is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "starttime", starg.starttime, 50); // Optional parameter.

  GetXMLBuffer(strxmlbuffer, "incfield", starg.incfield, 30); // Optional parameter.

  GetXMLBuffer(strxmlbuffer, "incfilename", starg.incfilename, 300); // Optional parameter.

  GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout); // Timeout time for the process heartbeat.
  if (starg.timeout == 0) { logfile.Write("timeout is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50); // Process name.
  if (strlen(starg.pname) == 0) { logfile.Write("pname is null.\n"); return false; }

  // 1. Parse starg.fieldlen into the ifieldlen array.
  CCmdStr CmdStr;

  // 1. Parse starg.fieldlen into the ifieldlen array.
  CmdStr.SplitToCmd(starg.fieldlen, ",");

  // Check if the number of fields exceeds the MAXFIELDCOUNT limit.
  if (CmdStr.CmdCount() > MAXFIELDCOUNT)
  {
    logfile.Write("The number of fields in fieldlen is too many, exceeding the maximum limit of %d.\n", MAXFIELDCOUNT);
    return false;
  }

  for (int ii = 0; ii < CmdStr.CmdCount(); ii++)
  {
    CmdStr.GetValue(ii, &ifieldlen[ii]);
    // if (ifieldlen[ii] > MAXFIELDLEN) ifieldlen[ii] = MAXFIELDLEN; // The field length cannot exceed MAXFIELDLEN.
    if (ifieldlen[ii] > MAXFIELDLEN) MAXFIELDLEN = ifieldlen[ii]; // Get the maximum field length.
  }

  ifieldcount = CmdStr.CmdCount();

  // 2. Parse starg.fieldstr into the strfieldname array.
  CmdStr.SplitToCmd(starg.fieldstr, ",");

  // Check if the number of fields exceeds the MAXFIELDCOUNT limit.
  if (CmdStr.CmdCount() > MAXFIELDCOUNT)
  {
    logfile.Write("The number of fields in fieldstr is too many, exceeding the maximum limit of %d.\n", MAXFIELDCOUNT);
    return false;
  }

  for (int ii = 0; ii < CmdStr.CmdCount(); ii++)
  {
    CmdStr.GetValue(ii, strfieldname[ii], 30);
  }

  // Check if the fields in strfieldname and ifieldlen arrays correspond.
  if (ifieldcount != CmdStr.CmdCount())
  {
    logfile.Write("The number of elements in fieldstr and fieldlen does not match.\n");
    return false;
  }

  // 3. Get the position of the incremental field in the result set.
  if (strlen(starg.incfield) != 0)
  {
    for (int ii = 0; ii < ifieldcount; ii++)
      if (strcmp(starg.incfield, strfieldname[ii]) == 0) { incfieldpos = ii; break; }

    if (incfieldpos == -1)
    {
      logfile.Write("The incremental field name %s is not in the list of %s.\n", starg.incfield, starg.fieldstr);
      return false;
    }
  }

  return true;
}


// Check if the current time is within the program's running time interval.
bool instarttime()
{
  // Program's running time interval, e.g., "02,13" means the program should run at 02:00 and 13:00, and not run at other times.
  if (strlen(starg.starttime) != 0)
  {
    char strHH24[3];
    memset(strHH24, 0, sizeof(strHH24));
    LocalTime(strHH24, "hh24"); // Get only the current hour from the current time.
    if (strstr(starg.starttime, strHH24) == 0)
      return false;
  }

  return true;
}

void crtxmlfilename() // Generate XML file name.
{
  // XML full file path = start.outpath + starg.bfilename + current time + starg.efilename + sequence number + .xml
  char strLocalTime[21];
  memset(strLocalTime, 0, sizeof(strLocalTime));
  LocalTime(strLocalTime, "yyyymmddhh24miss");

  static int iseq = 1;
  SNPRINTF(strxmlfilename, 300, sizeof(strxmlfilename), "%s/%s_%s_%s_%d.xml", starg.outpath, starg.bfilename, strLocalTime, starg.efilename, iseq++);
}

// Read the maximum id of the already extracted data from starg.incfilename file.
bool readincfile()
{
  imaxincvalue = 0; // The maximum value of the auto-increment field.

  // If starg.incfield parameter is empty, it means there is no incremental extraction.
  if (strlen(starg.incfield) == 0)
    return true;

  CFile File;

  // If opening starg.incfilename file fails, it means it is the first time running the program, so no need to return failure.
  // It may also be that the file is missing, in that case, nothing can be done but re-extract the data.
  if (File.Open(starg.incfilename, "r") == false)
    return true;

  // Read the maximum id of the already extracted data from the file.
  char strtemp[31];
  File.FFGETS(strtemp, 30);

  imaxincvalue = atol(strtemp);

  logfile.Write("Last extracted data position (%s=%ld).\n", starg.incfield, imaxincvalue);

  return true;
}

// Write the maximum id of the already extracted data into starg.incfilename file.
bool writeincfile()
{
  // If starg.incfield parameter is empty, it means there is no incremental extraction.
  if (strlen(starg.incfield) == 0)
    return true;

  CFile File;

  if (File.Open(starg.incfilename, "w+") == false)
  {
    logfile.Write("File.Open(%s) failed.\n", starg.incfilename);
    return false;
  }

  // Write the maximum id of the already extracted data into the file.
  File.Fprintf("%ld", imaxincvalue);

  File.Close();

  return true;
}


