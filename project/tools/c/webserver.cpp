/*
 * Program Name: webserver.cpp
 * This program is the server-side program of the data service bus.
 * It has added thread pool monitoring functionality.
 * Author: Wu Congzhou
 */
#include "_public.h"
#include "_ooci.h"

CLogFile logfile;   // Running log of the service program.
CTcpServer TcpServer; // Create a server-side object.

void EXIT(int sig); // Process exit function.

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Initialize the mutex.
pthread_cond_t cond = PTHREAD_COND_INITIALIZER; // Initialize the condition variable.
vector<int> sockqueue; // Client socket queue.

// Thread information structure.
struct st_pthinfo
{
  pthread_t pthid; // Thread ID.
  time_t atime; // Last activity time.
};

pthread_spinlock_t spin; // Spin lock for vthid.
vector<struct st_pthinfo> vthid; // Container for all thread information.
void *thmain(void *arg); // Worker thread main function.

pthread_t checkpthid;
void *checkthmain(void *arg); // Monitor thread main function.

void thcleanup(void *arg); // Thread cleanup function.

pthread_t checkpoolid;
void *checkpool(void *arg); // Database connection pool checking thread function.

// Main program argument structure.
struct st_arg
{
  char connstr[101]; // Database connection parameters.
  char charset[51]; // Database character set.
  int port; // Web service listening port.
} starg;

// Display program help.
void _help(char *argv[]);

// Parse XML into the parameter starg structure.
bool _xmltoarg(char *strxmlbuffer);

// Read client messages.
int ReadT(const int sockfd, char *buffer, const int size, const int itimeout);

// Get parameters from the GET request.
bool getvalue(const char *buffer, const char *name, char *value, const int len);

// Check username and password in the URL, if incorrect, return authentication failed response message.
bool Login(connection *conn, const char *buffer, const int sockfd);

// Check if the user has permission to call the interface, if not, return no permission response message.
bool CheckPerm(connection *conn, const char *buffer, const int sockfd);

// Execute the SQL statement of the interface and return data to the client.
bool ExecSQL(connection *conn, const char *buffer, const int sockfd);

// Database connection pool class.
class connpool
{
private:
  struct st_conn
  {
    connection conn; // Database connection.
    pthread_mutex_t mutex; // Mutex for the database connection.
    time_t atime; // Last time the database connection was used, 0 if not connected to the database.
  } *m_conns; // Database connection pool.

  int m_maxconns; // Maximum number of database connections in the pool.
  int m_timeout; // Database connection timeout time in seconds.
  char m_connstr[101]; // Database connection parameters: username/password@connection_name
  char m_charset[101]; // Database character set.
public:
  connpool(); // Constructor.
  ~connpool(); // Destructor.

  // Initialize the database connection pool, initialize locks, and return false if there are issues with the database connection parameters.
  bool init(const char *connstr, const char *charset, int maxconns, int timeout);
  // Disconnect the database connection, destroy locks, and release the memory space of the database connection pool.
  void destroy();

  // Get a free connection from the database connection pool, return the address of the database connection if successful.
  // If the connection pool is full or fails to connect to the database, return NULL.
  connection *get();
  // Return the database connection.
  bool free(connection *conn);

  // Check the database connection pool and disconnect idle connections. In the service program, this function is called by a dedicated sub-thread.
  void checkpool();
};

connpool oraconnpool; // Declare a database connection pool object.

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    _help(argv);
    return -1;
  }

  // Close all signals and input/output.
  // Set signals so that the process can be terminated gracefully using "kill + process_id" in the shell.
  // But please don't force termination using "kill -9 + process_id".
  CloseIOAndSignal();
  signal(SIGINT, EXIT);
  signal(SIGTERM, EXIT);

  if (logfile.Open(argv[1], "a+") == false)
  {
    printf("logfile.Open(%s) failed.\n", argv[1]);
    return -1;
  }

  // Parse XML into the parameter starg structure.
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

  // Start 10 worker threads, the number of threads is slightly more than the number of CPU cores.
  for (int ii = 0; ii < 10; ii++)
  {
    struct st_pthinfo stpthinfo;
    if (pthread_create(&stpthinfo.pthid, NULL, thmain, (void *)(long)ii) != 0)
    {
      logfile.Write("pthread_create() failed.\n");
      return -1;
    }

    stpthinfo.atime = time(0); // Set the activity time of the thread to the current time.
    vthid.push_back(stpthinfo); // Save the thread structure to the vthid container.
  }

  // Create the monitoring thread.
  if (pthread_create(&checkpthid, NULL, checkthmain, (void *)0) != 0)
  {
    logfile.Write("pthread_create() failed.\n");
    return -1;
  }

  pthread_spin_init(&spin, 0); // Initialize the spin lock for vthid.

  while (true)
  {
    // Wait for client connection requests.
    if (TcpServer.Accept() == false)
    {
      logfile.Write("TcpServer.Accept() failed.\n");
      return -1;
    }

    logfile.Write("Client (%s) connected.\n", TcpServer.GetIP());

    // Add the client socket to the queue and signal the condition.
    pthread_mutex_lock(&mutex); // Lock.
    sockqueue.push_back(TcpServer.m_connfd); // Enqueue.
    pthread_mutex_unlock(&mutex); // Unlock.
    pthread_cond_signal(&cond); // Trigger the condition and activate a thread.
  }
}

// Worker thread main function.
void *thmain(void *arg)
{
  int pthnum = (int)(long)arg; // Thread number.

  pthread_cleanup_push(thcleanup, arg); // Thread cleanup function.

  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); // Thread cancellation type is immediate cancellation.

  pthread_detach(pthread_self()); // Detach the thread.

  int connfd; // Client socket.
  char strrecvbuf[1024]; // Buffer to receive client request message.
  char strsendbuf[1024]; // Buffer to send response message to the client.

  while (true)
  {
    pthread_mutex_lock(&mutex); // Lock the cache queue.

    // If the cache queue is empty, wait. Use while to prevent condition variable spurious wakeup.
    while (sockqueue.size() == 0)
    {
      struct timeval now;
      gettimeofday(&now, NULL); // Get the current time.
      now.tv_sec = now.tv_sec + 20; // Get the time 20 seconds later.
      pthread_cond_timedwait(&cond, &mutex, (struct timespec *)&now); // Wait for the condition to be triggered.
      vthid[pthnum].atime = time(0); // Update the activity time of the current thread.
    }

    // Get the first record from the cache queue and then delete it.
    connfd = sockqueue[0];
    sockqueue.erase(sockqueue.begin());

    pthread_mutex_unlock(&mutex); // Unlock the cache queue.

    // The following code is to process the business logic.
    logfile.Write("Thread ID=%lu(Number=%d), connfd=%d\n", pthread_self(), pthnum, connfd);

    // Read client message, close the client socket and continue to the loop if timeout or failed.
    memset(strrecvbuf, 0, sizeof(strrecvbuf));
    if (ReadT(connfd, strrecvbuf, sizeof(strrecvbuf), 3) <= 0)
    {
      close(connfd);
      continue;
    }

    // If it's not a GET request message, don't process it, close the client socket, and continue to the loop.
    if (strncmp(strrecvbuf, "GET", 3) != 0)
    {
      close(connfd);
      continue;
    }

    logfile.Write("%s\n", strrecvbuf);

    connection *conn = oraconnpool.get(); // Get a database connection.

    // If the database connection is empty, return internal error to the client, close the client socket, and continue to the loop.
    if (conn == 0)
    {
      memset(strsendbuf, 0, sizeof(strsendbuf));
      sprintf(strsendbuf, \
              "HTTP/1.1 200 OK\r\n"\
              "Server: webserver\r\n"\
              "Content-Type: text/html;charset=utf-8\r\n\r\n"\
              "<retcode>-1</retcode><message>Internal error.</message>");
      Writen(connfd, strsendbuf, strlen(strsendbuf));
      close(connfd);
      continue;
    }

    // Check username and password in the URL, if incorrect, return authentication failed response message to the client, close the client socket, and continue to the loop.
    if (Login(conn, strrecvbuf, connfd) == false)
    {
      oraconnpool.free(conn);
      close(connfd);
      continue;
    }

    // Check if the user has permission to call the interface, if not, return no permission response message to the client, close the client socket, and continue to the loop.
    if (CheckPerm(conn, strrecvbuf, connfd) == false)
    {
      oraconnpool.free(conn);
      close(connfd);
      continue;
    }

    // First send the response message header to the client.
    memset(strsendbuf, 0, sizeof(strsendbuf));
    sprintf(strsendbuf, \
            "HTTP/1.1 200 OK\r\n"\
            "Server: webserver\r\n"\
            "Content-Type: text/html;charset=utf-8\r\n\r\n");
    Writen(connfd, strsendbuf, strlen(strsendbuf));

    // Execute the interface's SQL statement and return the data to the client.
    if (ExecSQL(conn, strrecvbuf, connfd) == false)
    {
      oraconnpool.free(conn);
      close(connfd);
      continue;
    }

    oraconnpool.free(conn);
  }

  pthread_cleanup_pop(1); // Pop the thread cleanup function.
}

// Process exit function.
void EXIT(int sig)
{
  // The following code is to prevent the signal handler from being interrupted by signals during execution.
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);

  logfile.Write("Process exits, sig=%d.\n", sig);

  TcpServer.CloseListen(); // Close the listening socket.

  // Cancel all threads.
  pthread_spin_lock(&spin);
  for (int ii = 0; ii < vthid.size(); ii++)
  {
    pthread_cancel(vthid[ii].pthid);
  }
  pthread_spin_unlock(&spin);

  sleep(1); // Give enough time for sub-threads to exit.

  pthread_cancel(checkpthid); // Cancel the monitoring thread.
  pthread_cancel(checkpoolid); // Cancel the database connection pool checking thread.

  pthread_spin_destroy(&spin);
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);

  exit(0);
}


void thcleanup(void *arg) // Thread cleanup function.
{
  pthread_mutex_unlock(&mutex);

  // Remove the thread's structure from the container storing thread structures.
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

  logfile.Write("Thread %d(%lu) exited.\n", (int)(long)arg, pthread_self());
}

// Display program help.
void _help(char *argv[])
{
  printf("Using: /project/tools1/bin/webserver logfilename xmlbuffer\n\n");

  printf("Sample: /project/tools1/bin/procctl 10 /project/tools1/bin/webserver /log/idc/webserver.log \"<connstr>scott/tiger@snorcl11g_132</connstr><charset>Simplified Chinese_China.AL32UTF8</charset><port>8080</port>\"\n\n");

  printf("This program is the server-side program of the data service bus, providing data access interfaces in HTTP protocol for the data center.\n");
  printf("logfilename: The log file for this program's runtime.\n");
  printf("xmlbuffer: The program's parameters represented in XML, as follows:\n\n");

  printf("connstr: Database connection parameters in the format username/password@tnsname.\n");
  printf("charset: Database character set. This parameter should be consistent with the data source database, or there might be Chinese garbled characters.\n");
  printf("port: The port on which the web service listens.\n\n");
}

// Parse XML into the parameter starg structure.
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

// Read client messages.
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

// Check username and password in the URL, if incorrect, return authentication failed response message.
bool Login(connection *conn, const char *buffer, const int sockfd)
{
  char username[31], passwd[31];

  getvalue(buffer, "username", username, 30); // Get the username.
  getvalue(buffer, "passwd", passwd, 30);     // Get the password.

  // Query the T_USERINFO table to check if the username and password exist.
  sqlstatement stmt;
  stmt.connect(conn);
  stmt.prepare("select count(*) from T_USERINFO where username=:1 and passwd=:2 and rsts=1");
  stmt.bindin(1, username, 30);
  stmt.bindin(2, passwd, 30);
  int icount = 0;
  stmt.bindout(1, &icount);
  stmt.execute();
  stmt.next();

  if (icount == 0) // Authentication failed, return authentication failed response message.
  {
    char strbuffer[256];
    memset(strbuffer, 0, sizeof(strbuffer));

    sprintf(strbuffer, \
            "HTTP/1.1 200 OK\r\n"\
            "Server: webserver\r\n"\
            "Content-Type: text/html;charset=utf-8\r\n\r\n"\
            "<retcode>-1</retcode><message>Username or password is invalid</message>");
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


// Check if the user has permission to call the interface. If not, return a response message indicating no permission.
bool CheckPerm(connection *conn, const char *buffer, const int sockfd)
{
  char username[31], intername[30];

  getvalue(buffer, "username", username, 30);     // Get the username.
  getvalue(buffer, "intername", intername, 30);   // Get the interface name.

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
            "HTTP/1.1 200 OK\r\n"\
            "Server: webserver\r\n"\
            "Content-Type: text/html;charset=utf-8\r\n\r\n"\
            "<retcode>-1</retcode><message>Permission denied</message>");

    Writen(sockfd, strbuffer, strlen(strbuffer));

    return false;
  }

  return true;
}


// Execute the interface's SQL statement and return the data to the client.
bool ExecSQL(connection *conn, const char *buffer, const int sockfd)
{
  // Parse the interface name from the request message.
  char intername[30];
  memset(intername, 0, sizeof(intername));
  getvalue(buffer, "intername", intername, 30);  // Get the interface name.

  // Load interface parameters from the T_INTERCFG table.
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
  stmt.execute();  // There's almost no need to check the return value here; errors are very unlikely.
  stmt.next();


  // Prepare the SQL statement for querying data.
  stmt.prepare(selectsql);

  // Split the input parameters bindin.
  CCmdStr CmdStr;
  CmdStr.SplitToCmd(bindin, ",");

  // Declare an array to hold input parameter values. Input parameter values are not too long, 100 is sufficient.
  char invalue[CmdStr.CmdCount()][101];
  memset(invalue, 0, sizeof(invalue));

  // Parse the input parameters from the HTTP GET request message and bind them to the SQL.
  for (int ii = 0; ii < CmdStr.CmdCount(); ii++)
  {
    getvalue(buffer, CmdStr.m_vCmdStr[ii].c_str(), invalue[ii], 100);
    stmt.bindin(ii + 1, invalue[ii], 100);
  }

  //////////////////////////////////////////////////

  // Bind the output variables of the SQL statement for querying data.
  // Based on the column names in the interface configuration (colstr field), bind the result set.
  //////////////////////////////////////////////////
  // Split colstr to obtain the number of fields in the result set.
  CmdStr.SplitToCmd(colstr, ",");

  // Array to hold the result set.
  char colvalue[CmdStr.CmdCount()][2001];

  // Bind the result set to the colvalue array.
  for (int ii = 0; ii < CmdStr.CmdCount(); ii++)
  {
    stmt.bindout(ii + 1, colvalue[ii], 2000);
  }
  //////////////////////////////////////////////////

  // Execute the SQL statement.
  char strsendbuffer[4001]; // XML to be sent to the client.
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

  // Send the header tag <data> to the client for XML content.
  Writen(sockfd, "<data>\n", strlen("<data>\n"));

  // Fetch the result set, for each record fetched, concatenate XML message, and send it to the client.
  //////////////////////////////////////////////////
  char strtemp[2001]; // Temporary variable for concatenating XML.



  // Fetch the result set row by row and send it to the client.
  while (true)
  {
    memset(strsendbuffer, 0, sizeof(strsendbuffer));
    memset(colvalue, 0, sizeof(colvalue));

    if (stmt.next() != 0) break; // Fetch one record from the result set.

    // Concatenate XML for each field.
    for (int ii = 0; ii < CmdStr.CmdCount(); ii++)
    {
      memset(strtemp, 0, sizeof(strtemp));
      snprintf(strtemp, 2000, "<%s>%s</%s>", CmdStr.m_vCmdStr[ii].c_str(), colvalue[ii], CmdStr.m_vCmdStr[ii].c_str());
      strcat(strsendbuffer, strtemp);
    }

    strcat(strsendbuffer, "<endl/>\n"); // XML end-of-line flag.

    Writen(sockfd, strsendbuffer, strlen(strsendbuffer)); // Send this row of data to the client.
  }
  //////////////////////////////////////////////////

  // Send the footer tag </data> of XML content to the client.
  Writen(sockfd, "</data>\n", strlen("</data>\n"));

  logfile.Write("intername=%s,count=%d\n", intername, stmt.m_cda.rpc);

  // Write to interface invocation log table T_USERLOG.

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

// Initialize the database connection pool, initialize locks, and return false if there is an issue with the database connection parameters.
bool connpool::init(const char *connstr, const char *charset, const int maxconns, int timeout)     
{
  // Try connecting to the database to validate the database connection parameters.
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
    pthread_mutex_init(&m_conns[ii].mutex, 0);    // Initialize locks.
    m_conns[ii].atime = 0;                       // Initialize the last usage time of the database connection to 0.
  }

  return true;
}


connpool::~connpool()
{
  destroy();
}

// Disconnect the database connections, destroy locks, and release the connection pool.
void connpool::destroy()
{
  for (int ii = 0; ii < m_maxconns; ii++)
  {
    m_conns[ii].conn.disconnect();             // Disconnect the database connection.
    pthread_mutex_destroy(&m_conns[ii].mutex); // Destroy the lock.
  }

  delete[] m_conns;         // Release the memory space of the database connection pool.
  m_conns = 0;

  memset(m_connstr, 0, sizeof(m_connstr));
  memset(m_charset, 0, sizeof(m_charset));
  m_maxconns = 0;
  m_timeout = 0;
}

// 1) Find an idle and already connected connection from the database connection pool. If found, return its address.
// 2) If not found, find an unconnected connection from the connection pool, connect to the database, and if successful, return the connection's address.
// 3) If an unconnected connection is found in step 2), but connecting to the database fails, return NULL.
// 4) If step 2) does not find an unconnected connection, it means that the database connection pool is full, also return NULL.
connection *connpool::get()
{
  int pos = -1;       // Used to record the first array position of an unconnected database connection.

  for (int ii = 0; ii < m_maxconns; ii++)
  {
    if (pthread_mutex_trylock(&m_conns[ii].mutex) == 0)  // Try to acquire the lock.
    {
      if (m_conns[ii].atime > 0)        // If the database connection is already connected.
      {
        printf("Obtained connection %d.\n", ii);
        m_conns[ii].atime = time(0);    // Set the database connection's usage time to the current time.
        return &m_conns[ii].conn;       // Return the address of the database connection.
      }

      if (pos == -1)
        pos = ii;   // Record the first array position of an unconnected database connection.
      else
        pthread_mutex_unlock(&m_conns[ii].mutex);  // Release the lock.
    }
  }

  if (pos == -1)   // If the connection pool is full, return NULL.
  {
    printf("Connection pool is full.\n");
    return NULL;
  }

  // If the connection pool is not full, let m_conns[pos].conn connect to the database.
  printf("New connection %d.\n");

  // Connect to the database.
  if (m_conns[pos].conn.connecttodb(m_connstr, m_charset) != 0)
  {
    // If connecting to the database fails, release the lock and return NULL.
    printf("Failed to connect to the database.\n");
    pthread_mutex_unlock(&m_conns[pos].mutex);  // Release the lock.
    return NULL;
  }

  m_conns[pos].atime = time(0);      // Set the database connection's usage time to the current time.

  return &m_conns[pos].conn;
}


// Return the database connection to the pool.
bool connpool::free(connection *conn) 
{
  for (int ii = 0; ii < m_maxconns; ii++)
  {
    if (&m_conns[ii].conn == conn)
    {
      printf("Returning connection %d\n", ii);
      m_conns[ii].atime = time(0);      // Set the database connection's usage time to the current time.
      pthread_mutex_unlock(&m_conns[ii].mutex);        // Release the lock.
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
    if (pthread_mutex_trylock(&m_conns[ii].mutex) == 0)  // Try to acquire the lock.
    {
      if (m_conns[ii].atime > 0)    // If it is an available connection.
      {
        // Check if the connection has timed out.
        if ((time(0) - m_conns[ii].atime) > m_timeout) 
        {
          printf("Connection %d has timed out.\n", ii);
          m_conns[ii].conn.disconnect();     // Disconnect the database connection.
          m_conns[ii].atime = 0;               // Reset the database connection's usage time.
        }
        else  
        {
          // If the connection has not timed out, execute a test SQL to verify if the connection is valid. If it's invalid, disconnect it.
          // If the network is disconnected or the database is restarted, we just need to disconnect the connection, the reconnection is handled by the get() function.
          if (m_conns[ii].conn.execute("select * from dual") != 0)
          {
            printf("Connection %d is faulty.\n", ii);
            m_conns[ii].conn.disconnect();     // Disconnect the database connection.
            m_conns[ii].atime = 0;               // Reset the database connection's usage time.
          }
        }
      }

      pthread_mutex_unlock(&m_conns[ii].mutex);   // Release the lock.
    }
    
    // If trying to acquire the lock fails, it means the database connection is currently in use, no need to check.
  }
}


void *checkpool(void *arg)    // Thread function to check the database connection pool.
{
  while (true)
  {
    oraconnpool.checkpool();
    sleep(30);
  }
}

void *checkthmain(void *arg)    // Main function for the monitoring thread.
{
  while (true)
  {
    // Iterate through the container of worker thread structures and check if each worker thread has timed out.
    for (int ii = 0; ii < vthid.size(); ii++)
    {
      // Worker threads have a timeout of 20 seconds, here we use 25 seconds for timeout check, which is sufficient.
      if ((time(0) - vthid[ii].atime) > 25)
      {
        // It has timed out.
        logfile.Write("Thread %d(%lu) timed out (%d seconds).\n", ii, vthid[ii].pthid, time(0) - vthid[ii].atime);

        // Cancel the timed-out worker thread.
        pthread_cancel(vthid[ii].pthid);

        // Re-create the worker thread.
        if (pthread_create(&vthid[ii].pthid, NULL, thmain, (void *)(long)ii) != 0)
        {
          logfile.Write("pthread_create() failed.\n");
          EXIT(-1);
        }

        vthid[ii].atime = time(0);      // Set the activity time of the worker thread.
      }
    }

    sleep(3);
  } 
}










