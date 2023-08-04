/*
 * Program name: tcpgetfiles.cpp, using TCP protocol, implements a client for file downloading.
*/
#include "_public.h"

// Structure for program's running parameters.
struct st_arg
{
  int  clienttype;          // Client type, 1 - upload file; 2 - download file.
  char ip[31];              // Server's IP address.
  int  port;                // Server's port.
  int  ptype;               // Processing type of the server file after successful download: 1 - delete file; 2 - move to backup directory.
  char srvpath[301];        // Root directory for server files.
  char srvpathbak[301];     // Root directory for backing up successfully downloaded files on the server (valid when ptype == 2).
  bool andchild;            // Whether to download files in all subdirectories under srvpath, true - yes; false - no.
  char matchname[301];      // File name matching rule for files to be downloaded, e.g., "*.TXT,*.XML".
  char clientpath[301];     // Root directory for client files.
  int  timetvl;             // Time interval for scanning server directory files, in seconds.
  int  timeout;             // Timeout for process heartbeat.
  char pname[51];           // Process name, recommended to use "tcpgetfiles_" suffix.
} starg;

CLogFile logfile;

// Function for program exit and signal 2, 15 handling.
void EXIT(int sig);

void _help();

// Parse XML and populate the starg structure with parameters.
bool _xmltoarg(char* strxmlbuffer);

CTcpClient TcpClient;

bool Login(const char* argv);    // Login business.

char strrecvbuffer[1024];   // Buffer for sending messages.
char strsendbuffer[1024];   // Buffer for receiving messages.

// Main function for file download.
void _tcpgetfiles();

// Receive file content.
bool RecvFile(const int sockfd, const char* filename, const char* mtime, int filesize);

CPActive PActive;  // Process heartbeat.

int main(int argc, char* argv[])
{
  if (argc != 3) { _help(); return -1; }

  // Close all signals and input/output.
  // Set signals, "kill + process number" can terminate these processes in shell status.
  // However, do not use "kill -9 +process number" to force termination.
  CloseIOAndSignal(); signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

  // Open the log file.
  if (logfile.Open(argv[1], "a+") == false)
  {
    printf("Failed to open the log file (%s).\n", argv[1]);
    return -1;
  }

  // Parse XML and get the program's running parameters.
  if (_xmltoarg(argv[2]) == false) return -1;

  PActive.AddPInfo(starg.timeout, starg.pname);  // Write process heartbeat information to shared memory.

  // Initiate a connection request to the server.
  if (TcpClient.ConnectToServer(starg.ip, starg.port) == false)
  {
    logfile.Write("TcpClient.ConnectToServer(%s, %d) failed.\n", starg.ip, starg.port);
    EXIT(-1);
  }

  // Login business.
  if (Login(argv[2]) == false) { logfile.Write("Login() failed.\n"); EXIT(-1); }

  // Call the main function for file download.
  _tcpgetfiles();

  EXIT(0);
}

// Login business.
bool Login(const char* argv)
{
  memset(strsendbuffer, 0, sizeof(strsendbuffer));
  memset(strrecvbuffer, 0, sizeof(strrecvbuffer));

  SPRINTF(strsendbuffer, sizeof(strsendbuffer), "%s<clienttype>2</clienttype>", argv);
  logfile.Write("Sending: %s\n", strsendbuffer);
  if (TcpClient.Write(strsendbuffer) == false) return false; // Send request message to the server.

  if (TcpClient.Read(strrecvbuffer, 20) == false) return false; // Receive response message from the server.
  logfile.Write("Received: %s\n", strrecvbuffer);

  logfile.Write("Login (%s:%d) successful.\n", starg.ip, starg.port);

  return true;
}

void EXIT(int sig)
{
  logfile.Write("Program exit, sig=%d\n\n", sig);
  exit(0);
}


void _help()
{
  printf("\n");
  printf("Using:/project/tools1/bin/tcpgetfiles logfilename xmlbuffer\n\n");

  printf("Sample:/project/tools1/bin/procctl 20 /project/tools1/bin/tcpgetfiles /log/idc/tcpgetfiles_surfdata.log \"<ip>192.168.174.132</ip><port>5005</port><ptype>1</ptype><srvpath>/tmp/tcp/surfdata2</srvpath><andchild>true</andchild><matchname>*.XML,*.CSV,*.JSON</matchname><clientpath>/tmp/tcp/surfdata3</clientpath><timetvl>10</timetvl><timeout>50</timeout><pname>tcpgetfiles_surfdata</pname>\"\n");
  printf("       /project/tools1/bin/procctl 20 /project/tools1/bin/tcpgetfiles /log/idc/tcpgetfiles_surfdata.log \"<ip>192.168.174.132</ip><port>5005</port><ptype>2</ptype><srvpath>/tmp/tcp/surfdata2</srvpath><srvpathbak>/tmp/tcp/surfdata2bak</srvpathbak><andchild>true</andchild><matchname>*.XML,*.CSV,*.JSON</matchname><clientpath>/tmp/tcp/surfdata3</clientpath><timetvl>10</timetvl><timeout>50</timeout><pname>tcpgetfiles_surfdata</pname>\"\n\n\n");

  printf("This program is a common module of the data center, using TCP protocol to download files from the server.\n");
  printf("logfilename   The log file for this program to run.\n");
  printf("xmlbuffer     The parameters for running this program are as follows:\n");
  printf("ip            The server's IP address.\n");
  printf("port          The server's port.\n");
  printf("ptype         Processing type of the server file after successful download: 1- delete file; 2- move to backup directory.\n");
  printf("srvpath       Root directory for server files.\n");
  printf("srvpathbak    Root directory for backing up successfully downloaded files on the server (valid when ptype == 2).\n");
  printf("andchild      Whether to download files in all subdirectories under srvpath, true - yes; false - no, default is false.\n");
  printf("matchname     File name matching rule for files to be downloaded, e.g., \"*.TXT,*.XML\"\n");
  printf("clientpath    Root directory for client files.\n");
  printf("timetvl       Time interval for scanning server directory files, in seconds, value between 1 and 30.\n");
  printf("timeout       Timeout for this program, in seconds, depending on file size and network bandwidth, it is recommended to set it above 50.\n");
  printf("pname         Process name, recommended to use \"tcpgetfiles_\" suffix for easy troubleshooting.\n\n");
}

// Parse XML and populate the starg structure with parameters.
bool _xmltoarg(char* strxmlbuffer)
{
  memset(&starg, 0, sizeof(struct st_arg));

  GetXMLBuffer(strxmlbuffer, "ip", starg.ip);
  if (strlen(starg.ip) == 0) { logfile.Write("ip is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "port", &starg.port);
  if (starg.port == 0) { logfile.Write("port is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "ptype", &starg.ptype);
  if ((starg.ptype != 1) && (starg.ptype != 2)) { logfile.Write("ptype not in (1,2).\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "srvpath", starg.srvpath);
  if (strlen(starg.srvpath) == 0) { logfile.Write("srvpath is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "srvpathbak", starg.srvpathbak);
  if ((starg.ptype == 2) && (strlen(starg.srvpathbak) == 0)) { logfile.Write("srvpathbak is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "andchild", &starg.andchild);

  GetXMLBuffer(strxmlbuffer, "matchname", starg.matchname);
  if (strlen(starg.matchname) == 0) { logfile.Write("matchname is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "clientpath", starg.clientpath);
  if (strlen(starg.clientpath) == 0) { logfile.Write("clientpath is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer, "timetvl", &starg.timetvl);
  if (starg.timetvl == 0) { logfile.Write("timetvl is null.\n"); return false; }

  // Time interval for scanning server directory files, in seconds.
  // starg.timetvl does not need to exceed 30 seconds.
  if (starg.timetvl > 30) starg.timetvl = 30;

  // Timeout for process heartbeat, must be greater than starg.timetvl, no need to be less than 50 seconds.
  GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);
  if (starg.timeout == 0) { logfile.Write("timeout is null.\n"); return false; }
  if (starg.timeout < 50) starg.timeout = 50;

  GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50);
  if (strlen(starg.pname) == 0) { logfile.Write("pname is null.\n"); return false; }

  return true;
}


// Main function for file downloading.
void _tcpgetfiles()
{
  PActive.AddPInfo(starg.timeout, starg.pname);

  while (true)
  {
    memset(strsendbuffer, 0, sizeof(strsendbuffer));
    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));

    PActive.UptATime();

    // Receive messages from the server.
    // The second parameter must be greater than starg.timetvl and less than starg.timeout.
    if (TcpClient.Read(strrecvbuffer, starg.timetvl + 10) == false)
    {
      logfile.Write("TcpClient.Read() failed.\n");
      return;
    }

    // logfile.Write("strrecvbuffer=%s\n",strrecvbuffer);

    // Handle heartbeat message.
    if (strcmp(strrecvbuffer, "<activetest>ok</activetest>") == 0)
    {
      strcpy(strsendbuffer, "ok");
      // logfile.Write("strsendbuffer=%s\n",strsendbuffer);
      if (TcpClient.Write(strsendbuffer) == false)
      {
        logfile.Write("TcpClient.Write() failed.\n");
        return;
      }
    }

    // Handle download file request message.
    if (strncmp(strrecvbuffer, "<filename>", 10) == 0)
    {
      // Parse the xml of the download file request message.
      char serverfilename[301];
      memset(serverfilename, 0, sizeof(serverfilename));
      char mtime[21];
      memset(mtime, 0, sizeof(mtime));
      int filesize = 0;
      GetXMLBuffer(strrecvbuffer, "filename", serverfilename, 300);
      GetXMLBuffer(strrecvbuffer, "mtime", mtime, 19);
      GetXMLBuffer(strrecvbuffer, "size", &filesize);

      // The client and server file directories are different.
      // The following code generates the client's file name.
      // Replace srvpath with clientpath in the file name, be cautious with the third parameter.
      char clientfilename[301];
      memset(clientfilename, 0, sizeof(clientfilename));
      strcpy(clientfilename, serverfilename);
      UpdateStr(clientfilename, starg.srvpath, starg.clientpath, false);

      // Receive file content.
      logfile.Write("recv %s(%d) ...", clientfilename, filesize);
      if (RecvFile(TcpClient.m_connfd, clientfilename, mtime, filesize) == true)
      {
        logfile.WriteEx("ok.\n");
        SNPRINTF(strsendbuffer, sizeof(strsendbuffer), 1000, "<filename>%s</filename><result>ok</result>", serverfilename);
      }
      else
      {
        logfile.WriteEx("failed.\n");
        SNPRINTF(strsendbuffer, sizeof(strsendbuffer), 1000, "<filename>%s</filename><result>failed</result>", serverfilename);
      }

      // Send the receiving result back to the server.
      // logfile.Write("strsendbuffer=%s\n",strsendbuffer);
      if (TcpClient.Write(strsendbuffer) == false)
      {
        logfile.Write("TcpClient.Write() failed.\n");
        return;
      }
    }
  }
}

// Function to receive the content of a file.
bool RecvFile(const int sockfd, const char *filename, const char *mtime, int filesize)
{
  // Generate the temporary file name.
  char strfilenametmp[301];
  SNPRINTF(strfilenametmp, sizeof(strfilenametmp), 300, "%s.tmp", filename);

  int totalbytes = 0; // Total number of received file bytes.
  int onread = 0;     // Number of bytes intended to be received this time.
  char buffer[1000];  // Buffer to receive file content.
  FILE *fp = NULL;

  // Create the temporary file.
  if ((fp = FOPEN(strfilenametmp, "wb")) == NULL)
    return false;

  while (true)
  {
    memset(buffer, 0, sizeof(buffer));

    // Calculate the number of bytes to be received this time.
    if (filesize - totalbytes > 1000)
      onread = 1000;
    else
      onread = filesize - totalbytes;

    // Receive file content.
    if (Readn(sockfd, buffer, onread) == false)
    {
      fclose(fp);
      return false;
    }

    // Write the received content to the file.
    fwrite(buffer, 1, onread, fp);

    // Calculate the total number of received file bytes and break the loop if the file is fully received.
    totalbytes = totalbytes + onread;

    if (totalbytes == filesize)
      break;
  }

  // Close the temporary file.
  fclose(fp);

  // Reset the file's modified time.
  UTime(strfilenametmp, mtime);

  // Rename the temporary file to the official file.
  if (RENAME(strfilenametmp, filename) == false)
    return false;

  return true;
}








