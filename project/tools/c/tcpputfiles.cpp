/*
 * Program name: tcpputfiles.cpp
 * TCP protocol-based client for file uploading.
*/
#include "_public.h"

// Structure for program running parameters.
struct st_arg
{
  int  clienttype;          // Client type: 1 - file upload; 2 - file download.
  char ip[31];              // Server IP address.
  int  port;                // Server port.
  int  ptype;               // Local file handling after successful upload: 1 - delete file; 2 - move to backup directory.
  char clientpath[301];     // Root directory for local file storage.
  char clientpathbak[301];  // Root directory for backup of successfully uploaded files (valid when ptype == 2).
  bool andchild;            // Whether to upload files from subdirectories of clientpath: true - yes; false - no.
  char matchname[301];      // Matching rule for filenames to upload, e.g., "*.TXT,*.XML".
  char srvpath[301];        // Root directory for server file storage.
  int  timetvl;             // Time interval for scanning local directory files, in seconds.
  int  timeout;             // Timeout for process heartbeat, in seconds.
  char pname[51];           // Process name, it is recommended to use "tcpputfiles_suffix" format.
} starg;

CLogFile logfile;

// Function prototypes for program exit and signal handling.
void EXIT(int sig);
void _help();

// Parse XML to st_arg structure.
bool _xmltoarg(char *strxmlbuffer);

CTcpClient TcpClient;

bool Login(const char *argv);    // Login business.

bool ActiveTest();    // Heartbeat.

char strrecvbuffer[1024];   // Buffer for sending messages.
char strsendbuffer[1024];   // Buffer for receiving messages.

// Main function for file uploading, executes one file upload task.
bool _tcpputfiles();
bool bcontinue = true;   // If _tcpputfiles sent files, bcontinue is true, initialized as true.

// Send the content of the file to the remote end.
bool SendFile(const int sockfd, const char *filename, const int filesize);

// Delete or move the local file.
bool AckMessage(const char *strrecvbuffer);

CPActive PActive;  // Process heartbeat.

int main(int argc, char *argv[])
{
  if (argc != 3) { _help(); return -1; }

  // Close all signals and input/output.
  // Set signals to allow termination with "kill + process ID" in shell mode.
  // But please avoid using "kill -9 +process ID" to force termination.
  CloseIOAndSignal(); signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

  // Open the log file.
  if (logfile.Open(argv[1], "a+") == false)
  {
    printf("Failed to open the log file (%s).\n", argv[1]);
    return -1;
  }

  // Parse XML to get the program's running parameters.
  if (_xmltoarg(argv[2]) == false) return -1;

  PActive.AddPInfo(starg.timeout, starg.pname);  // Write process heartbeat information into shared memory.

  // Establish a connection request to the server.
  if (TcpClient.ConnectToServer(starg.ip, starg.port) == false)
  {
    logfile.Write("TcpClient.ConnectToServer(%s,%d) failed.\n", starg.ip, starg.port);
    EXIT(-1);
  }

  // Login business.
  if (Login(argv[2]) == false)
  {
    logfile.Write("Login() failed.\n");
    EXIT(-1);
  }

  while (true)
  {
    // Call the main function for file uploading, executing one file upload task.
    if (_tcpputfiles() == false)
    {
      logfile.Write("_tcpputfiles() failed.\n");
      EXIT(-1);
    }

    if (bcontinue == false)
    {
      sleep(starg.timetvl);

      if (ActiveTest() == false)
        break;
    }

    PActive.UptATime();
  }
   
  EXIT(0);
}

// Heartbeat.
bool ActiveTest()
{
  memset(strsendbuffer, 0, sizeof(strsendbuffer));
  memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
 
  SPRINTF(strsendbuffer, sizeof(strsendbuffer), "<activetest>ok</activetest>");
  // logfile.Write("Sent: %s\n", strsendbuffer);
  if (TcpClient.Write(strsendbuffer) == false) return false; // Send request message to the server.

  if (TcpClient.Read(strrecvbuffer, 20) == false) return false; // Receive response message from the server.
  // logfile.Write("Received: %s\n", strrecvbuffer);

  return true;
}

// Login business.
bool Login(const char *argv)
{
  memset(strsendbuffer, 0, sizeof(strsendbuffer));
  memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
 
  SPRINTF(strsendbuffer, sizeof(strsendbuffer), "%s<clienttype>1</clienttype>", argv);
  logfile.Write("Sent: %s\n", strsendbuffer);
  if (TcpClient.Write(strsendbuffer) == false) return false; // Send request message to the server.

  if (TcpClient.Read(strrecvbuffer, 20) == false) return false; // Receive response message from the server.
  logfile.Write("Received: %s\n", strrecvbuffer);

  logfile.Write("Login(%s:%d) successful.\n", starg.ip, starg.port); 

  return true;
}

void EXIT(int sig)
{
  logfile.Write("Program exit, sig=%d\n\n", sig);

  exit(0);
}


vvoid _help()
{
  printf("\n");
  printf("Usage: /project/tools1/bin/tcpputfiles logfilename xmlbuffer\n\n");

  printf("Sample: /project/tools1/bin/procctl 20 /project/tools1/bin/tcpputfiles /log/idc/tcpputfiles_surfdata.log \"<ip>192.168.174.133</ip><port>5005</port><ptype>1</ptype><clientpath>/tmp/tcp/surfdata1</clientpath><clientpathbak>/tmp/tcp/surfdata1bak</clientpathbak><andchild>true</andchild><matchname>*.XML,*.CSV,*.JSON</matchname><srvpath>/tmp/tcp/surfdata2</srvpath><timetvl>10</timetvl><timeout>50</timeout><pname>tcpputfiles_surfdata</pname>\"\n");
  printf("       /project/tools1/bin/procctl 20 /project/tools1/bin/tcpputfiles /log/idc/tcpputfiles_surfdata.log \"<ip>192.168.174.132</ip><port>5005</port><ptype>2</ptype><clientpath>/tmp/tcp/surfdata1</clientpath><clientpathbak>/tmp/tcp/surfdata1bak</clientpathbak><andchild>true</andchild><matchname>*.XML,*.CSV,*.JSON</matchname><srvpath>/tmp/tcp/surfdata2</srvpath><timetvl>10</timetvl><timeout>50</timeout><pname>tcpputfiles_surfdata</pname>\"\n\n\n");

  printf("This program is a common function module in the data center, using TCP protocol to send files to the server.\n");
  printf("logfilename   The log file for program running.\n");
  printf("xmlbuffer     The parameters for program running in XML format, as follows:\n");
  printf("ip            The server's IP address.\n");
  printf("port          The server's port.\n");
  printf("ptype         The handling method after successful file upload: 1 - delete file; 2 - move to backup directory.\n");
  printf("clientpath    The root directory for local file storage.\n");
  printf("clientpathbak The root directory for backup of successfully uploaded files (valid when ptype == 2).\n");
  printf("andchild      Whether to upload files from subdirectories of clientpath: true - yes; false - no; defaults to false.\n");
  printf("matchname     The matching rule for filenames to upload, e.g., \"*.TXT,*.XML\"\n");
  printf("srvpath       The root directory for server file storage.\n");
  printf("timetvl       The time interval for scanning local directory files, in seconds, ranging from 1 to 30.\n");
  printf("timeout       The timeout for this program, in seconds, depends on file size and network bandwidth, recommended to set above 50.\n");
  printf("pname         The process name, use a clear and distinct name from other processes to facilitate troubleshooting.\n\n");
}

// Parse XML to st_arg structure
bool _xmltoarg(char *strxmlbuffer)
{
  memset(&starg, 0, sizeof(struct st_arg));

  GetXMLBuffer(strxmlbuffer, "ip", starg.ip);
  if (strlen(starg.ip) == 0) { logfile.Write("ip is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "port", &starg.port);
  if (starg.port == 0) { logfile.Write("port is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "ptype", &starg.ptype);
  if ((starg.ptype != 1) && (starg.ptype != 2)) { logfile.Write("ptype not in (1,2).\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "clientpath", starg.clientpath);
  if (strlen(starg.clientpath) == 0) { logfile.Write("clientpath is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "clientpathbak", starg.clientpathbak);
  if ((starg.ptype == 2) && (strlen(starg.clientpathbak) == 0)) { logfile.Write("clientpathbak is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "andchild", &starg.andchild);

  GetXMLBuffer(strxmlbuffer, "matchname", starg.matchname);
  if (strlen(starg.matchname) == 0) { logfile.Write("matchname is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "srvpath", starg.srvpath);
  if (strlen(starg.srvpath) == 0) { logfile.Write("srvpath is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "timetvl", &starg.timetvl);
  if (starg.timetvl == 0) { logfile.Write("timetvl is null.\n"); return false; }

  // Time interval for scanning local directory files, in seconds.
  // starg.timetvl doesn't need to exceed 30 seconds.
  if (starg.timetvl > 30) starg.timetvl = 30;

  // Timeout for process heartbeat, must be greater than starg.timetvl, no need to be less than 50 seconds.
  GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);
  if (starg.timeout == 0) { logfile.Write("timeout is null.\n"); return false; }
  if (starg.timeout < 50) starg.timeout = 50;

  GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50);
  if (strlen(starg.pname) == 0) { logfile.Write("pname is null.\n"); return false; }

  return true;
}

// Main function for file uploading, executes one file upload task.
bool _tcpputfiles()
{
  CDir Dir;

  // Call OpenDir() to open the starg.clientpath directory.
  if (Dir.OpenDir(starg.clientpath, starg.matchname, 10000, starg.andchild) == false)
  {
    logfile.Write("Dir.OpenDir(%s) failed.\n", starg.clientpath);
    return false;
  }

  int delayed = 0; // Number of files that have not received confirmation message from the server.
  int buflen = 0;   // Buffer length for strrecvbuffer.

  bcontinue = false;

  while (true)
  {
    memset(strsendbuffer, 0, sizeof(strsendbuffer));
    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));

    // Traverse each file in the directory, call ReadDir() to get a filename.
    if (Dir.ReadDir() == false) break;

    bcontinue = true;

    // Compose a message with filename, modification time, and file size, and send it to the server.
    SNPRINTF(strsendbuffer, sizeof(strsendbuffer), 1000, "<filename>%s</filename><mtime>%s</mtime><size>%d</size>", Dir.m_FullFileName, Dir.m_ModifyTime, Dir.m_FileSize);

    // logfile.Write("strsendbuffer=%s\n", strsendbuffer);
    if (TcpClient.Write(strsendbuffer) == false)
    {
      logfile.Write("TcpClient.Write() failed.\n");
      return false;
    }

    // Send the content of the file to the server.
    logfile.Write("send %s(%d) ...", Dir.m_FullFileName, Dir.m_FileSize);
    if (SendFile(TcpClient.m_connfd, Dir.m_FullFileName, Dir.m_FileSize) == true)
    {
      logfile.WriteEx("ok.\n");
      delayed++;
    }
    else
    {
      logfile.WriteEx("failed.\n");
      TcpClient.Close();
      return false;
    }

    PActive.UptATime();

    // Receive confirmation messages from the server.
    while (delayed > 0)
    {
      memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
      if (TcpRead(TcpClient.m_connfd, strrecvbuffer, &buflen, -1) == false) break;
      // logfile.Write("strrecvbuffer=%s\n", strrecvbuffer);

      // Delete or move local files.
      delayed--;
      AckMessage(strrecvbuffer);
    }
  }

  // Continue receiving confirmation messages from the server.
  while (delayed > 0)
  {
    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
    if (TcpRead(TcpClient.m_connfd, strrecvbuffer, &buflen, 10) == false) break;
    // logfile.Write("strrecvbuffer=%s\n", strrecvbuffer);

    // Delete or move local files.
    delayed--;
    AckMessage(strrecvbuffer);
  }

  return true;
}



// Send the content of the file to the server.
bool SendFile(const int sockfd, const char *filename, const int filesize)
{
  int onread = 0;         // Number of bytes to read each time fread is called.
  int bytes = 0;          // Number of bytes read from the file in one fread call.
  char buffer[1000];      // Buffer to store the read data.
  int totalbytes = 0;     // Total number of bytes read from the file.
  FILE *fp = NULL;

  // Open the file in "rb" mode.
  if ((fp = fopen(filename, "rb")) == NULL)
    return false;

  while (true)
  {
    memset(buffer, 0, sizeof(buffer));

    // Calculate the number of bytes to read in this iteration.
    // If the remaining data is more than 1000 bytes, read 1000 bytes.
    if (filesize - totalbytes > 1000)
      onread = 1000;
    else
      onread = filesize - totalbytes;

    // Read data from the file.
    bytes = fread(buffer, 1, onread, fp);

    // Send the read data to the server.
    if (bytes > 0)
    {
      if (Writen(sockfd, buffer, bytes) == false)
      {
        fclose(fp);
        return false;
      }
    }

    // Update the total number of bytes read from the file.
    // If the entire file has been read, break out of the loop.
    totalbytes = totalbytes + bytes;
    if (totalbytes == filesize)
      break;
  }

  fclose(fp);
  return true;
}

// Delete or move local files.
bool AckMessage(const char *strrecvbuffer)
{
  char filename[301];
  char result[11];

  memset(filename, 0, sizeof(filename));
  memset(result, 0, sizeof(result));

  GetXMLBuffer(strrecvbuffer, "filename", filename, 300);
  GetXMLBuffer(strrecvbuffer, "result", result, 10);

  // If the server did not receive the file successfully, return directly.
  if (strcmp(result, "ok") != 0)
    return true;

  // ptype==1, delete the file.
  if (starg.ptype == 1)
  {
    if (REMOVE(filename) == false)
    {
      logfile.Write("REMOVE(%s) failed.\n", filename);
      return false;
    }
  }

  // ptype==2, move the file to the backup directory.
  if (starg.ptype == 2)
  {
    // Generate the filename for backup after moving.
    char bakfilename[301];
    STRCPY(bakfilename, sizeof(bakfilename), filename);
    UpdateStr(bakfilename, starg.clientpath, starg.clientpathbak, false);
    if (RENAME(filename, bakfilename) == false)
    {
      logfile.Write("RENAME(%s,%s) failed.\n", filename, bakfilename);
      return false;
    }
  }

  return true;
}













