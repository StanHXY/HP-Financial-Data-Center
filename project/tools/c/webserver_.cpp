/*
 * Program Name: webserver_.cpp
 * This program is the server program of the Data Service Bus, using a persistent connection.
 */
#include "_public.h"
#include "_ooci.h"

CLogFile logfile;       // Running log of the service program.
CTcpServer TcpServer;   // Create a server object.

void EXIT(int sig);     // Process exit function.

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Initialize the mutex.
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;    // Initialize the condition variable.
vector<int> sockqueue;                            // Queue for client sockets.
/*
struct st_clientinfo
{
  int sockid;    // Client socket.
  char ip[16];   // Client IP address.
};
*/

// Thread information structure.
struct st_pthread_info
{
  pthread_t pthid;    // Thread ID.
  time_t atime;       // Time of the last activity.
};

pthread_spinlock_t spin;         // Spin lock for vthid.
vector<struct st_pthread_info> vthid; // Container to store all thread information.
void *thmain(void *arg);         // Main function of the worker thread.

pthread_t checkpthid;
void *checkthmain(void *arg);    // Main function of the monitoring thread.

void thcleanup(void *arg);       // Thread cleanup function.

pthread_t checkpoolid;
void *checkpool(void *arg);      // Thread function to check the database connection pool.

// Main program parameter structure.
struct st_arg
{
  char connstr[101];  // Database connection parameters.
  char charset[51];   // Database character set.
  int port;           // Port for web service listening.
} starg;

// Display the program help.
void _help(char *argv[]);

// Parse XML into the starg structure.
bool _xmltoarg(char *strxmlbuffer);

// Read client messages.
int ReadT(const int sockfd, char *buffer, const int size, const int itimeout);

// Get parameters from the GET request.
bool getvalue(const char *buffer, const char *name, char *value, const int len);

// Check the username and password in the URL, return authentication failure response if incorrect.
bool Login(connection *conn, const char *buffer, const int sockfd);

// Check if the user has permission to call the interface, return no permission response if not.
bool CheckPerm(connection *conn, const char *buffer, const int sockfd);

// Execute the SQL statement of the interface and return the data to the client.
bool ExecSQL(connection *conn, const char *buffer, const int sockfd);

// Database connection pool class.
class connpool
{
private:
  struct st_conn
  {
    connection conn;            // Database connection.
    pthread_mutex_t mutex;      // Mutex for database connection.
    time_t atime;               // Time of the last use of the database connection, 0 if not connected to the database.
  } *m_conns;                   // Database connection pool.

  int m_maxconns;               // Maximum size of the database connection pool.
  int m_timeout;                // Database connection timeout, in seconds.
  char m_connstr[101];          // Database connection parameters: username/password@connection name.
  char m_charset[101];          // Database character set.
public:
  connpool();      // Constructor.
  ~connpool();      // Destructor.

  // Initialize the database connection pool, initialize the lock, return false if there is a problem with the database connection parameters.
  bool init(const char *connstr, const char *charset, int maxconns, int timeout);
  // Disconnect the database connection, destroy the lock, release the memory space of the database connection pool.
  void destroy();

  // Get an idle connection from the database connection pool, return the address of the database connection if successful.
  // Return null if the connection pool is exhausted or the connection to the database fails.
  connection *get();
  // Return the database connection.
  bool free(connection *conn);

  // Check the database connection pool, disconnect idle connections.
  // In the service program, use a dedicated sub-thread to call this function.
  void checkpool();
};

connpool oraconnpool;  // Declare the database connection pool object.

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    _help(argv);
    return -1;
  }

  // Close all signals and input/output.
  // Set signals to allow termination of processes using "kill + process ID" in shell mode.
  // But please do not use "kill -9 + process ID" to forcefully terminate them.
  CloseIOAndSignal();
  signal(SIGINT, EXIT);
  signal(SIGTERM, EXIT);

  if (logfile.Open(argv[1], "a+") == false)
  {
    printf("logfile.Open(%s) failed.\n", argv[1]);
    return -1;
  }

  // Parse XML into the starg structure.
  if (_xmltoarg(argv[2]) == false)
    return -1;

  // Server initialization.
  if (TcpServer.InitServer(starg.port) == false)
  {
    logfile.Write("TcpServer.InitServer(%d) failed.\n", starg.port);
    return -1;
  }

  // Initialize the database connection pool.
  if (oraconnpool.init(starg.connstr, starg.charset, 10, 50) == false)
  {
    logfile.Write("oraconnpool.init() failed.\n");
    return -1;
  }
  else
  {
    // Create a thread to check the database connection pool.
    if (pthread_create(&checkpoolid, NULL, checkpool, 0) != 0)
    {
      logfile.Write("pthread_create() failed.\n");
      return -1;
    }
  }

  // Start 10 worker threads, slightly more than the number of CPU cores.
  for (int ii = 0; ii < 10; ii++)
  {
    struct st_pthread_info stpthinfo;
    if (pthread_create(&stpthinfo.pthid, NULL, thmain, (void *)(long)ii) != 0)
    {
      logfile.Write("pthread_create() failed.\n");
      return -1;
    }

    stpthinfo.atime = time(0);        // Set the activity time of the thread to the current time.
    vthid.push_back(stpthinfo);       // Save the thread structure to the vthid container.
  }

  // Create the monitoring thread.
  if (pthread_create(&checkpthid, NULL, checkthmain, (void *)0) != 0)
  {
    logfile.Write("pthread_create() failed.\n");
    return -1;
  }

  // If the blacklist or whitelist method is used, manage the blacklist and whitelist using parameter tables.
  // At the start of the service program, load the parameters in the table into the container.
  // During the runtime of the service program, use a dedicated thread to reload the parameters regularly,
  // and load them every few minutes is sufficient.

  pthread_spin_init(&spin, 0);       // Initialize the spin lock for vthid.

  while (true)
  {
    // Wait for a client connection request.
    if (TcpServer.Accept() == false)
    {
      logfile.Write("TcpServer.Accept() failed.\n");
      return -1;
    }

    logfile.Write("Client (%s) has connected.\n", TcpServer.GetIP());

    // Add the client's socket to the queue and send a condition signal.
    pthread_mutex_lock(&mutex);               // Lock.
    sockqueue.push_back(TcpServer.m_connfd);  // Enqueue.
    pthread_mutex_unlock(&mutex);             // Unlock.
    pthread_cond_signal(&cond);               // Trigger the condition, activate a thread.
  }
}


// Worker thread main function.
void *thmain(void *arg)
{
  int pthnum = (int)(long)arg; // Thread number.

  pthread_cleanup_push(thcleanup, arg); // Thread cleanup function.

  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); // Thread cancellation type: immediate cancellation.

  pthread_detach(pthread_self()); // Detach the thread.

  int connfd;                   // Client socket.
  char strrecvbuf[1024];        // Buffer to receive client request messages.
  char strsendbuf[1024];        // Buffer to send response messages to the client.

  while (true)
  {
    pthread_mutex_lock(&mutex); // Lock the cache queue.

    // If the cache queue is empty, wait. Use 'while' to prevent spurious wakeups.
    while (sockqueue.size() == 0)
    {
      struct timeval now;
      gettimeofday(&now, NULL);                 // Get the current time.
      now.tv_sec = now.tv_sec + 20;             // Get the time after 20 seconds.
      pthread_cond_timedwait(&cond, &mutex, (struct timespec *)&now); // Wait for the condition to be triggered.
      vthid[pthnum].atime = time(0);           // Update the activity time of the current thread.
    }

    // Get the first record from the cache queue and then delete it.
    connfd = sockqueue[0];
    sockqueue.erase(sockqueue.begin());

    pthread_mutex_unlock(&mutex); // Unlock the cache queue.

    // The following is the business processing code.
    logfile.Write("phid=%lu(num=%d),connfd=%d\n", pthread_self(), pthnum, connfd);

    while (true)
    {
      // Read the client's message, close the client's socket and continue to the loop if timeout or fails.
      memset(strrecvbuf, 0, sizeof(strrecvbuf));

      int iret = 0;
      iret = ReadT(connfd, strrecvbuf, sizeof(strrecvbuf), 20);
      if (iret == 0)
      {
        logfile.Write("timeout\n");
        vthid[pthnum].atime = time(0);
        continue;
      }
      if (iret < 0)
      {
        logfile.Write("disconnect\n");
        vthid[pthnum].atime = time(0);
        close(connfd);
        break;
      }

      // Do not process if it is not a GET request message, close the client's socket and continue to the loop.
      if (strncmp(strrecvbuf, "GET", 3) != 0)
      {
        close(connfd);
        break;
      }

      logfile.Write("%s\n", strrecvbuf);

      connection *conn = oraconnpool.get(); // Get a database connection.

      // If the database connection is empty, return internal error to the client, close the client's socket, and continue to the loop.
      if (conn == 0)
      {
        memset(strsendbuf, 0, sizeof(strsendbuf));
        sprintf(strsendbuf, \
          "HTTP/1.1 200 OK\r\n"\
          "Server: webserver_\r\n"\
          "Content-Type: text/html;charset=utf-8\r\n\r\n"\
          "<retcode>-1</retcode><message>internal error.</message>");
        Writen(connfd, strsendbuf, strlen(strsendbuf));
        close(connfd);
        break;
      }

      // Check the username and password in the URL, return authentication failure to the client, close the client's socket, and continue to the loop.
      if (Login(conn, strrecvbuf, connfd) == false)
      {
        oraconnpool.free(conn);
        close(connfd);
        break;
      }

      // Check if the user has permission to call the interface, return no permission to the client, close the client's socket, and continue to the loop.
      if (CheckPerm(conn, strrecvbuf, connfd) == false)
      {
        oraconnpool.free(conn);
        close(connfd);
        break;
      }

      // First, send the response message header to the client.
      memset(strsendbuf, 0, sizeof(strsendbuf));
      sprintf(strsendbuf, \
        "HTTP/1.1 200 OK\r\n"\
        "Server: webserver_\r\n"\
        "Content-Type: text/html;charset=utf-8\r\n\r\n");
      Writen(connfd, strsendbuf, strlen(strsendbuf));

      // Then execute the interface's SQL statement and return the data to the client.
      if (ExecSQL(conn, strrecvbuf, connfd) == false)
      {
        oraconnpool.free(conn);
        close(connfd);
        break;
      }

      oraconnpool.free(conn);;
    }

    close(connfd);
  }

  pthread_cleanup_pop(1); // Pop the thread cleanup function.
}

// Process exit function.
void EXIT(int sig)
{
  // The following code is to prevent the signal handling function from being interrupted by signals during execution.
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);

  logfile.Write("Process exit, sig=%d.\n", sig);

  TcpServer.CloseListen(); // Close the listening socket.

  // Cancel all threads.
  pthread_spin_lock(&spin);
  for (int ii = 0; ii < vthid.size(); ii++)
  {
    pthread_cancel(vthid[ii].pthid);
  }
  pthread_spin_unlock(&spin);

  sleep(1); // Give enough time for the child threads to exit.

  pthread_cancel(checkpthid);  // Cancel the monitoring thread.
  pthread_cancel(checkpoolid); // Cancel the thread checking the database connection pool.

  pthread_spin_destroy(&spin);
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);

  exit(0);
}

void thcleanup(void *arg) // Thread cleanup function.
{
  pthread_mutex_unlock(&mutex);

  // Remove the thread's structure from the container that stores thread structures.
  pthread_spin_lock(&spin);
  for (int ii = 0; ii < vthid.size(); ii++)
  {
    if (pthread_equal(pthread_self(), vthid[ii].pthid))
    {
      vthid.erase(vthid.begin() + ii);
      break;
    }
  }
  pthread_spin_unlock(&spin);

  logfile.Write("Thread %d (%lu) exits.\n", (int)(long)arg, pthread_self());
}

// Display the program's help.
void _help(char *argv[])
{
  printf("Using: %s logfilename xmlbuffer\n\n", argv[0]);
  printf("Sample: %s 10 /project/tools1/bin/webserver_ /log/idc/webserver_.log \"<connstr>scott/tiger@snorcl11g_132</connstr><charset>Simplified Chinese_China.AL32UTF8</charset><port>8080</port>\"\n\n", argv[0]);

  printf("This program is the server program of the Data Service Bus, providing HTTP protocol data access interfaces for the data center.\n");
  printf("logfilename: The log file for this program's execution.\n");
  printf("xmlbuffer: The program's runtime parameters represented in XML format, as follows:\n\n");

  printf("connstr: The database connection parameters in the format: username/password@tnsname.\n");
  printf("charset: The database character set, which should be consistent with the data source database to avoid Chinese garbled characters.\n");
  printf("port: The port for web service listening.\n\n");
}

// Parse XML to the starg structure.
bool _xmltoarg(char *strxmlbuffer)
{
  memset(&starg, 0, sizeof(struct st_arg));

  GetXMLBuffer(strxmlbuffer, "connstr", starg.connstr, 100);
  if (strlen(starg.connstr) == 0)
  {
    logfile.Write("connstr is null.\n");
    return false;
  }

  GetXMLBuffer(strxmlbuffer, "charset", starg.charset, 50);
  if (strlen(starg.charset) == 0)
  {
    logfile.Write("charset is null.\n");
    return false;
  }

  GetXMLBuffer(strxmlbuffer, "port", &starg.port);
  if (starg.port == 0)
  {
    logfile.Write("port is null.\n");
    return false;
  }

  return true;
}


// Read the client's message.
int ReadT(const int sockfd, char *buffer, const int size, const int itimeout)
{
  if (itimeout > 0)
  {
    struct pollfd fds;
    fds.fd = sockfd;
    fds.events = POLLIN;
    int iret;
    if ((iret = poll(&fds, 1, itimeout * 1000)) <= 0)
      return iret;
  }

  return recv(sockfd, buffer, size, 0);
}

// Check if the username and password in the URL are correct, return authentication failure response message if not.
bool Login(connection *conn, const char *buffer, const int sockfd)
{
  char username[31], passwd[31];

  getvalue(buffer, "username", username, 30); // Get the username.
  getvalue(buffer, "passwd", passwd, 30);     // Get the password.

  // Query T_USERINFO table to check if the username and password exist.
  sqlstatement stmt;
  stmt.connect(conn);
  stmt.prepare("select count(*) from T_USERINFO where username=:1 and passwd=:2 and rsts=1");
  stmt.bindin(1, username, 30);
  stmt.bindin(2, passwd, 30);
  int icount = 0;
  stmt.bindout(1, &icount);
  stmt.execute();
  stmt.next();

  if (icount == 0) // Authentication failed, return authentication failure response message.
  {
    char strbuffer[256];
    memset(strbuffer, 0, sizeof(strbuffer));

    sprintf(strbuffer, \
            "HTTP/1.1 200 OK\r\n" \
            "Server: webserver_\r\n" \
            "Content-Type: text/html;charset=utf-8\r\n\r\n" \
            "<retcode>-1</retcode><message>username or passwd is invalid</message>");
    Writen(sockfd, strbuffer, strlen(strbuffer));

    return false;
  }

  return true;
}

// Get parameters from the GET request.
bool getvalue(const char *buffer, const char *name, char *value, const int len)
{
  value[0] = 0;

  char *start, *end;
  start = end = 0;

  start = strstr((char *)buffer, (char *)name);
  if (start == 0)
    return false;

  end = strstr(start, "&");
  if (end == 0)
    end = strstr(start, " ");

  if (end == 0)
    return false;

  int ilen = end - (start + strlen(name) + 1);
  if (ilen > len)
    ilen = len;

  strncpy(value, start + strlen(name) + 1, ilen);

  value[ilen] = 0;

  return true;
}

// Check if the user has permission to call the interface, return no permission response message if not.
bool CheckPerm(connection *conn, const char *buffer, const int sockfd)
{
  char username[31], intername[30];

  getvalue(buffer, "username", username, 30);       // Get the username.
  getvalue(buffer, "intername", intername, 30);     // Get the interface name.

  sqlstatement stmt;
  stmt.connect(conn);
  stmt.prepare("select count(*) from T_USERANDINTER where username=:1 and intername=:2 and intername in (select intername from T_INTERCFG where rsts=1)");
  stmt.bindin(1, username, 30);
  stmt.bindin(2, intername, 30);
  int icount = 0;
  stmt.bindout(1, &icount);
  stmt.execute();
  stmt.next();

  if (icount != 1)
  {
    char strbuffer[256];
    memset(strbuffer, 0, sizeof(strbuffer));

    sprintf(strbuffer, \
            "HTTP/1.1 200 OK\r\n" \
            "Server: webserver_\r\n" \
            "Content-Type: text/html;charset=utf-8\r\n\n\n" \
            "<retcode>-1</retcode><message>permission denied</message>");

    Writen(sockfd, strbuffer, strlen(strbuffer));

    return false;
  }

  return true;
}

// Execute the interface's SQL statement and return data to the client.
bool ExecSQL(connection *conn, const char *buffer, const int sockfd)
{
  // Parse the interface name from the request message.
  char intername[30];
  memset(intername, 0, sizeof(intername));
  getvalue(buffer, "intername", intername, 30); // Get the interface name.

  // Load interface parameters from the T_INTERCFG interface parameter configuration table.
  char selectsql[1001], colstr[301], bindin[301];
  memset(selectsql, 0, sizeof(selectsql)); // Interface SQL.
  memset(colstr, 0, sizeof(colstr));       // Output column names.
  memset(bindin, 0, sizeof(bindin));       // Interface parameters.
  sqlstatement stmt;
  stmt.connect(conn);
  stmt.prepare("select selectsql, colstr, bindin from T_INTERCFG where intername=:1");
  stmt.bindin(1, intername, 30);    // Interface name.
  stmt.bindout(1, selectsql, 1000); // Interface SQL.
  stmt.bindout(2, colstr, 300);     // Output column names.
  stmt.bindout(3, bindin, 300);     // Interface parameters.
  stmt.execute(); // There is almost no need to check the return value here, as errors are very rare.
  stmt.next();

  // Prepare the SQL statement for querying data.
  stmt.prepare(selectsql);

  // http://192.168.174.132:8080?username=ty&passwd=typwd&intername=getzhobtmind3&obtid=59287&begintime=20211024094318&endtime=20211024113920
  // SQL statement: select obtid, to_char(ddatetime, 'yyyymmddhh24miss'), t, p, u, wd, wf, r, vis from T_ZHOBTMIND where obtid=:1 and ddatetime>=to_date(:2, 'yyyymmddhh24miss') and ddatetime<=to_date(:3, 'yyyymmddhh24miss')
  // colstr field: obtid, ddatetime, t, p, u, wd, wf, r, vis
  // bindin field: obtid, begintime, endtime

  // Bind input variables of the SQL statement for querying data.
  // Parse parameters from the URL based on the parameter list (bindin field) in the interface configuration and bind them to the SQL statement for querying data.
  //////////////////////////////////////////////////
  // Split the input parameter bindin.
  CCmdStr CmdStr;
  CmdStr.SplitToCmd(bindin, ",");

  // Declare an array to store input parameters. The input parameter values will not be too long, so 100 is enough.
  char invalue[CmdStr.CmdCount()][101];
  memset(invalue, 0, sizeof(invalue));

  // Parse input parameters from the http GET request message and bind them to the SQL.
  for (int ii = 0; ii < CmdStr.CmdCount(); ii++)
  {
    getvalue(buffer, CmdStr.m_vCmdStr[ii].c_str(), invalue[ii], 100);
    stmt.bindin(ii + 1, invalue[ii], 100);
  }
  //////////////////////////////////////////////////

  // Bind output variables of the SQL statement for querying data.
  // Based on the column names (colstr field) in the interface configuration, bind the output result set.
  //////////////////////////////////////////////////
  // Split colstr to get the number of fields in the result set.
  CmdStr.SplitToCmd(colstr, ",");

  // Declare an array to store the result set.
  char colvalue[CmdStr.CmdCount()][2001];

  // Bind the result set to the colvalue array.
  for (int ii = 0; ii < CmdStr.CmdCount(); ii++)
  {
    stmt.bindout(ii + 1, colvalue[ii], 2000);
  }
  //////////////////////////////////////////////////

  // Execute the SQL statement.
  char strsendbuffer[4001]; // Send XML to the client.
  memset(strsendbuffer, 0, sizeof(strsendbuffer));

  if (stmt.execute() != 0)
  {
    sprintf(strsendbuffer, "<retcode>%d</retcode><message>%s</message>\n", stmt.m_cda.rc, stmt.m_cda.message);
    Writen(sockfd, strsendbuffer, strlen(strsendbuffer));
    logfile.Write("stmt.execute() failed.\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message);
    return false;
  }
  strcpy(strsendbuffer, "<retcode>0</retcode><message>ok</message>\n");
  Writen(sockfd, strsendbuffer, strlen(strsendbuffer));

  // Send the XML header tag <data> to the client.
  Writen(sockfd, "<data>\n", strlen("<data>\n"));

  // Fetch the result set, concatenate XML, and send it to the client for each record.
  //////////////////////////////////////////////////
  char strtemp[2001]; // Temporary variable for concatenating XML.

  // Fetch each row of the result set and send it to the client.
  while (true)
  {
    memset(strsendbuffer, 0, sizeof(strsendbuffer));
    memset(colvalue, 0, sizeof(colvalue));

    if (stmt.next() != 0)
      break; // Fetch one record from the result set.

    // Concatenate XML for each field.
    for (int ii = 0; ii < CmdStr.CmdCount(); ii++)
    {
      memset(strtemp, 0, sizeof(strtemp));
      snprintf(strtemp, 2000, "<%s>%s</%s>", CmdStr.m_vCmdStr[ii].c_str(), colvalue[ii], CmdStr.m_vCmdStr[ii].c_str());
      strcat(strsendbuffer, strtemp);
    }

    strcat(strsendbuffer, "<endl/>\n"); // XML end-of-line tag.

    Writen(sockfd, strsendbuffer, strlen(strsendbuffer)); // Send this line of data to the client.
  }
  //////////////////////////////////////////////////

  // Send the XML closing tag </data> to the client.
  Writen(sockfd, "</data>\n", strlen("</data>\n"));

  logfile.Write("intername=%s,count=%d\n", intername, stmt.m_cda.rpc);

  // Write to the interface invocation log table T_USERLOG.

  return true;
}


connpool::connpool()
{
  m_maxconns=0;
  m_timeout=0;
  memset(m_connstr,0,sizeof(m_connstr));
  memset(m_charset,0,sizeof(m_charset));
  m_conns=0;
}

// Initialize the database connection pool, initialize locks, and return false if there are issues with the database connection parameters.
bool connpool::init(const char *connstr, const char *charset, const int maxconns, int timeout)
{
  // Try to connect to the database and validate the database connection parameters.
  connection conn;
  if (conn.connecttodb(connstr, charset) != 0)
  {
    printf("Failed to connect to the database.\n%s\n", conn.m_cda.message);
    return false;
  }
  conn.disconnect();

  strncpy(m_connstr, connstr, 100);
  strncpy(m_charset, charset, 100);
  m_maxconns = maxconns;
  m_timeout = timeout;

  // Allocate memory space for the database connection pool.
  m_conns = new struct st_conn[m_maxconns];

  for (int ii = 0; ii < m_maxconns; ii++)
  {
    pthread_mutex_init(&m_conns[ii].mutex, 0); // Initialize locks.
    m_conns[ii].atime = 0;                    // Initialize the last used time of the database connection to 0.
  }

  return true;
}

connpool::~connpool()
{
  destroy();
}

// Disconnect the database connection, destroy locks, and release the connection pool.
void connpool::destroy()
{
  for (int ii = 0; ii < m_maxconns; ii++)
  {
    m_conns[ii].conn.disconnect();             // Disconnect the database connection.
    pthread_mutex_destroy(&m_conns[ii].mutex); // Destroy locks.
  }

  delete[] m_conns; // Release the memory space of the database connection pool.
  m_conns = 0;

  memset(m_connstr, 0, sizeof(m_connstr));
  memset(m_charset, 0, sizeof(m_charset));
  m_maxconns = 0;
  m_timeout = 0;
}

// 1) Find a free and already connected connection from the database connection pool. If found, return its address.
// 2) If not found, find an unconnected connection in the connection pool, connect to the database, and return the address of the connection if successful.
// 3) If an unconnected connection is found in step 2) but fails to connect to the database, return NULL.
// 4) If no unconnected connection is found in step 2), it means that the database connection pool is full, and also return NULL.
connection *connpool::get()
{
  int pos = -1; // Used to record the position of the first unconnected database connection.

  for (int ii = 0; ii < m_maxconns; ii++)
  {
    if (pthread_mutex_trylock(&m_conns[ii].mutex) == 0) // Try to lock.
    {
      if (m_conns[ii].atime > 0) // If the database connection is already connected.
      {
        printf("Get connection %d.\n", ii);
        m_conns[ii].atime = time(0); // Set the database connection's last used time to the current time.
        return &m_conns[ii].conn;    // Return the address of the database connection.
      }

      if (pos == -1)
        pos = ii; // Record the position of the first unconnected database connection.
      else
        pthread_mutex_unlock(&m_conns[ii].mutex); // Release the lock.
    }
  }

  if (pos == -1) // If the connection pool is full, return NULL.
  {
    printf("Connection pool is full.\n");
    return NULL;
  }

  // If the connection pool is not full, let m_conns[pos].conn connect to the database.
  printf("New connection %d.\n", pos);

  // Connect to the database.
  if (m_conns[pos].conn.connecttodb(m_connstr, m_charset) != 0)
  {
    // If connecting to the database fails, release the lock and return NULL.
    printf("Failed to connect to the database.\n");
    pthread_mutex_unlock(&m_conns[pos].mutex); // Release the lock.
    return NULL;
  }

  m_conns[pos].atime = time(0); // Set the database connection's last used time to the current time.

  return &m_conns[pos].conn;
}

// Return the database connection.
bool connpool::free(connection *conn)
{
  for (int ii = 0; ii < m_maxconns; ii++)
  {
    if (&m_conns[ii].conn == conn)
    {
      printf("Return %d\n", ii);
      m_conns[ii].atime = time(0); // Set the database connection's last used time to the current time.
      pthread_mutex_unlock(&m_conns[ii].mutex); // Release the lock.
      return true;
    }
  }

  return false;
}

// Check the connection pool and disconnect idle connections.
void connpool::checkpool()
{
  for (int ii = 0; ii < m_maxconns; ii++)
  {
    if (pthread_mutex_trylock(&m_conns[ii].mutex) == 0) // Try to lock.
    {
      if (m_conns[ii].atime > 0) // If it's an available connection.
      {
        // Check if the connection has timed out.
        if ((time(0) - m_conns[ii].atime) > m_timeout)
        {
          printf("Connection %d has timed out.\n", ii);
          m_conns[ii].conn.disconnect(); // Disconnect the database connection.
          m_conns[ii].atime = 0;         // Reset the database connection's last used time.
        }
        else
        {
          // If it hasn't timed out, execute a SQL query to validate if the connection is valid. If it's not valid, disconnect it.
          // If the network is disconnected or the database is restarted, the reconnection operation will be handled by the get() function.
          if (m_conns[ii].conn.execute("select * from dual") != 0)
          {
            printf("Connection %d is faulty.\n", ii);
            m_conns[ii].conn.disconnect(); // Disconnect the database connection.
            m_conns[ii].atime = 0;         // Reset the database connection's last used time.
          }
        }
      }

      pthread_mutex_unlock(&m_conns[ii].mutex); // Release the lock.
    }

    // If the attempt to lock fails, it means the database connection is in use, no need to check.
  }
}

void *checkpool(void *arg) // Thread function to check the database connection pool.
{
  while (true)
  {
    oraconnpool.checkpool();
    sleep(30);
  }
}

void *checkthmain(void *arg) // Monitor thread main function.
{
  while (true)
  {
    // Traverse the container of worker thread structures and check if each worker thread has timed out.
    for (int ii = 0; ii < vthid.size(); ii++)
    {
      // The worker thread timeout is 20 seconds, here we use 25 seconds to judge the timeout, which is sufficient.
      if ((time(0) - vthid[ii].atime) > 25)
      {
        // Has timed out.
        logfile.Write("Thread %d(%lu) timed out(%d).\n", ii, vthid[ii].pthid, time(0) - vthid[ii].atime);

        // Cancel the timed-out worker thread.
        pthread_cancel(vthid[ii].pthid);

        // Recreate the worker thread.
        if (pthread_create(&vthid[ii].pthid, NULL, thmain, (void *)(long)ii) != 0)
        {
          logfile.Write("pthread_create() failed.\n");
          EXIT(-1);
        }

        vthid[ii].atime = time(0); // Set the activity time of the worker thread.
      }
    }

    sleep(3);
  }
}










