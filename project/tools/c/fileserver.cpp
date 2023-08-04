/*
 * File: fileserver.cpp, the server side of file transfer.
 */
#include "_public.h"

// Structure for program arguments.
struct st_arg
{
  int clienttype;          // Client type: 1-upload file; 2-download file.
  char ip[31];              // Server's IP address.
  int port;                // Server's port.
  int ptype;               // File processing method after successful upload: 1-delete file; 2-move to backup directory.
  char clientpath[301];     // Root directory for storing local files.
  char clientpathbak[301];  // Root directory for backing up local files after successful upload, valid when ptype==2.
  bool andchild;            // Whether to upload files in subdirectories of clientpath, true-yes; false-no.
  char matchname[301];      // Matching rule for files to be uploaded, e.g., "*.TXT,*.XML".
  char srvpath[301];        // Root directory for storing server-side files.
  int timetvl;             // Time interval for scanning local directory files, in seconds.
  int timeout;             // Process heartbeat timeout time.
  char pname[51];           // Process name, recommended to use "tcpgetfiles_suffix" format.
} starg;

// Parse XML and store the parameters in starg structure.
bool _xmltoarg(char *strxmlbuffer);

CLogFile logfile;      // Log file for the server program.
CTcpServer TcpServer;  // Create a server object.

void FathEXIT(int sig);  // Parent process exit function.
void ChldEXIT(int sig);  // Child process exit function.

char strrecvbuffer[1024];   // Buffer for receiving messages.
char strsendbuffer[1024];   // Buffer for sending messages.

// Login business processing function.
bool ClientLogin();

// Main function for uploading files.
void RecvFilesMain();

// Receive the content of the uploaded file.
bool RecvFile(const int sockfd, const char *filename, const char *mtime, int filesize);

CPActive PActive;  // Process heartbeat.

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    printf("Using: ./fileserver port logfile\nExample: ./fileserver 5005 /log/idc/fileserver.log\n\n");
    return -1;
  }

  // Close all signals and input/output.
  // Set the signal so that "kill + process number" can terminate the process normally in the shell.
  // But please do not use "kill -9 + process number" to force termination.
  CloseIOAndSignal();
  signal(SIGINT, FathEXIT);
  signal(SIGTERM, FathEXIT);

  if (logfile.Open(argv[2], "a+") == false)
  {
    printf("logfile.Open(%s) failed.\n", argv[2]);
    return -1;
  }

  // Server initialization.
  if (TcpServer.InitServer(atoi(argv[1])) == false)
  {
    logfile.Write("TcpServer.InitServer(%s) failed.\n", argv[1]);
    return -1;
  }

  while (true)
  {
    // Wait for client connection requests.
    if (TcpServer.Accept() == false)
    {
      logfile.Write("TcpServer.Accept() failed.\n");
      FathEXIT(-1);
    }

    logfile.Write("Client (%s) connected.\n", TcpServer.GetIP());

    // Process communication with the client in the child process.
    if (fork() > 0) 
    { 
      TcpServer.CloseClient(); // Parent process continues to Accept().
      continue; 
    }

    // Child process handles communication with the client.

    // Process the login message from the client.
    if (ClientLogin() == false)
      ChldEXIT(-1);

    // If clienttype==1, call the main function for uploading files.
    if (starg.clienttype == 1)
      RecvFilesMain();

    // If clienttype==2, call the main function for downloading files.

    ChldEXIT(0);
  }
}

// Parent process exit function.
void FathEXIT(int sig)
{
  // The following code is to prevent the signal processing function from being interrupted by the signal during execution.
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);

  logfile.Write("Parent process exited, sig=%d.\n", sig);

  TcpServer.CloseListen(); // Close the listening socket.

  kill(0, 15); // Notify all child processes to exit.

  exit(0);
}

// Child process exit function.
void ChldEXIT(int sig)
{
  // The following code is to prevent the signal processing function from being interrupted by the signal during execution.
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);

  logfile.Write("Child process exited, sig=%d.\n", sig);

  TcpServer.CloseClient(); // Close the client socket.

  exit(0);
}


// Login.
bool ClientLogin()
{
  memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
  memset(strsendbuffer, 0, sizeof(strsendbuffer));

  if (TcpServer.Read(strrecvbuffer, 20) == false)
  {
    logfile.Write("TcpServer.Read() failed.\n");
    return false;
  }
  logfile.Write("strrecvbuffer=%s\n", strrecvbuffer);

  // Parse the client login message.
  _xmltoarg(strrecvbuffer);

  if ((starg.clienttype != 1) && (starg.clienttype != 2))
    strcpy(strsendbuffer, "failed");
  else
    strcpy(strsendbuffer, "ok");

  if (TcpServer.Write(strsendbuffer) == false)
  {
    logfile.Write("TcpServer.Write() failed.\n");
    return false;
  }

  logfile.Write("%s login %s.\n", TcpServer.GetIP(), strsendbuffer);

  return true;
}

// Parse XML and store the parameters in starg structure.
bool _xmltoarg(char *strxmlbuffer)
{
  memset(&starg, 0, sizeof(struct st_arg));

  // No need to perform legality checks on the parameters, as the client has already done it.
  GetXMLBuffer(strxmlbuffer, "clienttype", &starg.clienttype);
  GetXMLBuffer(strxmlbuffer, "ptype", &starg.ptype);
  GetXMLBuffer(strxmlbuffer, "clientpath", starg.clientpath);
  GetXMLBuffer(strxmlbuffer, "andchild", &starg.andchild);
  GetXMLBuffer(strxmlbuffer, "matchname", starg.matchname);
  GetXMLBuffer(strxmlbuffer, "srvpath", starg.srvpath);

  GetXMLBuffer(strxmlbuffer, "timetvl", &starg.timetvl);
  if (starg.timetvl > 30)
    starg.timetvl = 30;

  GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);
  if (starg.timeout < 50)
    starg.timeout = 50;

  GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50);
  strcat(starg.pname, "_srv");

  return true;
}

// Main function for uploading files.
void RecvFilesMain()
{
  PActive.AddPInfo(starg.timeout, starg.pname);

  while (true)
  {
    memset(strsendbuffer, 0, sizeof(strsendbuffer));
    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));

    PActive.UptATime();

    // Receive client message.
    // The second parameter must be greater than starg.timetvl and less than starg.timeout.
    if (TcpServer.Read(strrecvbuffer, starg.timetvl + 10) == false)
    {
      logfile.Write("TcpServer.Read() failed.\n");
      return;
    }
    // logfile.Write("strrecvbuffer=%s\n",strrecvbuffer);

    // Process heartbeat message.
    if (strcmp(strrecvbuffer, "<activetest>ok</activetest>") == 0)
    {
      strcpy(strsendbuffer, "ok");
      // logfile.Write("strsendbuffer=%s\n",strsendbuffer);
      if (TcpServer.Write(strsendbuffer) == false)
      {
        logfile.Write("TcpServer.Write() failed.\n");
        return;
      }
    }

    // Process upload file request message.
    if (strncmp(strrecvbuffer, "<filename>", 10) == 0)
    {
      // Parse the xml of the upload file request message.
      char clientfilename[301];
      memset(clientfilename, 0, sizeof(clientfilename));
      char mtime[21];
      memset(mtime, 0, sizeof(mtime));
      int filesize = 0;
      GetXMLBuffer(strrecvbuffer, "filename", clientfilename, 300);
      GetXMLBuffer(strrecvbuffer, "mtime", mtime, 19);
      GetXMLBuffer(strrecvbuffer, "size", &filesize);

      // The client and server file directories are different, the following code generates the server-side file name.
      // Replace clientpath with srvpath in the file name, be careful with the third parameter.
      char serverfilename[301];
      memset(serverfilename, 0, sizeof(serverfilename));
      strcpy(serverfilename, clientfilename);
      UpdateStr(serverfilename, starg.clientpath, starg.srvpath, false);

      // Receive the content of the uploaded file.
      logfile.Write("recv %s(%d) ...", serverfilename, filesize);
      if (RecvFile(TcpServer.m_connfd, serverfilename, mtime, filesize) == true)
      {
        logfile.WriteEx("ok.\n");
        SNPRINTF(strsendbuffer, sizeof(strsendbuffer), 1000, "<filename>%s</filename><result>ok</result>", clientfilename);
      }
      else
      {
        logfile.WriteEx("failed.\n");
        SNPRINTF(strsendbuffer, sizeof(strsendbuffer), 1000, "<filename>%s</filename><result>failed</result>", clientfilename);
      }

      // Return the receiving result to the client.
      // logfile.Write("strsendbuffer=%s\n",strsendbuffer);
      if (TcpServer.Write(strsendbuffer) == false)
      {
        logfile.Write("TcpServer.Write() failed.\n");
        return;
      }
    }
  }
}

// Receive the content of the uploaded file.
bool RecvFile(const int sockfd, const char *filename, const char *mtime, int filesize)
{
  // Generate a temporary file name.
  char strfilenametmp[301];
  SNPRINTF(strfilenametmp, sizeof(strfilenametmp), 300, "%s.tmp", filename);

  int totalbytes = 0; // Total number of bytes received for the file.
  int onread = 0;     // Number of bytes to be received in this round.
  char buffer[1000];  // Buffer for receiving file content.
  FILE *fp = NULL;

  // Create a temporary file.
  if ((fp = FOPEN(strfilenametmp, "wb")) == NULL)
    return false;

  while (true)
  {
    memset(buffer, 0, sizeof(buffer));

    // Calculate the number of bytes to be received in this round.
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

    // Calculate the total number of bytes received for the file. If the file is fully received, break the loop.
    totalbytes = totalbytes + onread;

    if (totalbytes == filesize)
      break;
  }

  // Close the temporary file.
  fclose(fp);

  // Reset the file's modification time.
  UTime(strfilenametmp, mtime);

  // Rename the temporary file to the official file.
  if (RENAME(strfilenametmp, filename) == false)
    return false;

  return true;
}











