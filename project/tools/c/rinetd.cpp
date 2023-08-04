/*
 * Program Name: rinetd.cpp, Network Proxy Service Program - External Side.
*/
#include "_public.h"

// Structure for proxy route parameters.
struct st_route
{
  int  listenport;      // Local listening communication port.
  char dstip[31];       // IP address of the destination host.
  int  dstport;         // Communication port of the destination host.
  int  listensock;      // Local listening socket.
} stroute;
std::vector<struct st_route> vroute;       // Container for proxy routes.
bool loadroute(const char *inifile);  // Load proxy route parameters into the vroute container.

// Initialize the server's listening port.
int initserver(int port);

int epollfd = 0;  // epoll handle.
int tfd = 0;      // Timer handle.

#define MAXSOCK  1024
int clientsocks[MAXSOCK];       // Stores the value of each socket connection's remote socket.
int clientatime[MAXSOCK];       // Stores the last time each socket connection sent/received a message.

int cmdlistensock = 0;            // Server listens for incoming commands from internal clients.
int cmdconnsock = 0;              // Control channel between internal clients and the server.

void EXIT(int sig);   // Process exit function.

CLogFile logfile;

CPActive PActive;     // Process heartbeat.


int main(int argc, char* argv[])
{
  if (argc != 4)
  {
    printf("\n");
    printf("Usage: ./rinetd logfile inifile cmdport\n\n");
    printf("Example: ./rinetd /tmp/rinetd.log /etc/rinetd.conf 4000\n\n");
    printf("         /project/tools1/bin/procctl 5 /project/tools1/bin/rinetd /tmp/rinetd.log /etc/rinetd.conf 4000\n\n");
    printf("logfile: The log file name for this program's runtime logs.\n");
    printf("inifile: The configuration file for proxy service parameters.\n");
    printf("cmdport: The communication port with the internal network proxy program.\n\n");
    return -1;
  }

  // Close all signals and I/O.
  // Set signals, in shell, you can use "kill + process id" to terminate these processes normally.
  // But please do not use "kill -9 + process id" to forcefully terminate them.
  CloseIOAndSignal();
  signal(SIGINT, EXIT);
  signal(SIGTERM, EXIT);

  // Open the log file.
  if (logfile.Open(argv[1], "a+") == false)
  {
    printf("Failed to open the log file (%s).\n", argv[1]);
    return -1;
  }

  PActive.AddPInfo(30, "inetd"); // Set the process heartbeat timeout to 30 seconds.

  // Load proxy route parameters into the vroute container.
  if (loadroute(argv[2]) == false)
    return -1;

  logfile.Write("Successfully loaded proxy route parameters (%d).\n", vroute.size());

  // Initialize the listening port for internal network programs, wait for the internal network to initiate a connection, and establish the control channel.
  if ((cmdlistensock = initserver(atoi(argv[3]))) < 0)
  {
    logfile.Write("initserver(%s) failed.\n", argv[3]);
    EXIT(-1);
  }

  // Wait for the connection request from the internal network program and establish the control channel.
  struct sockaddr_in client;
  socklen_t len = sizeof(client);
  cmdconnsock = accept(cmdlistensock, (struct sockaddr*)&client, &len);
  if (cmdconnsock < 0)
  {
    logfile.Write("accept() failed.\n");
    EXIT(-1);
  }
  logfile.Write("Control channel with the internal network has been established (cmdconnsock=%d).\n", cmdconnsock);

  // Initialize the server's listening sockets.
  for (int ii = 0; ii < vroute.size(); ii++)
  {
    if ((vroute[ii].listensock = initserver(vroute[ii].listenport)) < 0)
    {
      logfile.Write("initserver(%d) failed.\n", vroute[ii].listenport);
      EXIT(-1);
    }

    // Set the listening socket as non-blocking.
    fcntl(vroute[ii].listensock, F_SETFL, fcntl(vroute[ii].listensock, F_GETFD, 0) | O_NONBLOCK);
  }

  // Create the epoll handle.
  epollfd = epoll_create(1);

  struct epoll_event ev; // Declare the event data structure.

  // Prepare readable events for listening to the external network sockets.
  for (int ii = 0; ii < vroute.size(); ii++)
  {
    ev.events = EPOLLIN;                // Read event.
    ev.data.fd = vroute[ii].listensock;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, vroute[ii].listensock, &ev); // Add the event of listening to the external network socket to epollfd.
  }

  // Note: cmdlistensock and cmdconnsock for listening to internal network programs are blocking and do not need to be managed by epoll.

  // Create the timer.
  tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC); // Create the timerfd.

  struct itimerspec timeout;
  memset(&timeout, 0, sizeof(struct itimerspec));
  timeout.it_value.tv_sec = 20; // Timeout after 20 seconds.
  timeout.it_value.tv_nsec = 0;
  timerfd_settime(tfd, 0, &timeout, NULL); // Set the timer.

  // Prepare the event for the timer.
  ev.events = EPOLLIN | EPOLLET; // Read event, note that ET mode must be used.
  ev.data.fd = tfd;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, tfd, &ev);

  PActive.AddPInfo(30, "rinetd"); // Set the process heartbeat timeout to 30 seconds.

  struct epoll_event evs[10]; // Store the events returned by epoll.

  while (true)
  {
    // Wait for events to occur on the monitored sockets.
    int infds = epoll_wait(epollfd, evs, 10, -1);

    // If epoll_wait returns an error.
    if (infds < 0)
    {
      logfile.Write("epoll() failed.\n");
      break;
    }

    // Traverse the array of events evs that have occurred.
    for (int ii = 0; ii < infds; ii++)
    {
      ////////////////////////////////////////////////////////
      // If the timer has timed out, send a heartbeat to the control channel, update the process heartbeat, and clean up idle client sockets.
      if (evs[ii].data.fd == tfd)
      {
        timerfd_settime(tfd, 0, &timeout, NULL); // Reset the timer.

        PActive.UptATime(); // Update the process heartbeat.

        // Send a heartbeat packet to the internal network program through the control channel.
        char buffer[256];
        strcpy(buffer, "<activetest>");
        if (send(cmdconnsock, buffer, strlen(buffer), 0) <= 0)
        {
          logfile.Write("Control channel with the internal network has been disconnected.\n");
          EXIT(-1);
        }

        for (int jj = 0; jj < MAXSOCK; jj++)
        {
          // Close the socket if it has been idle for more than 80 seconds.
          if ((clientsocks[jj] > 0) && ((time(0) - clientatime[jj]) > 80))
          {
            logfile.Write("Client (%d,%d) timed out.\n", clientsocks[jj], clientsocks[clientsocks[jj]]);
            close(clientsocks[jj]);
            close(clientsocks[clientsocks[jj]]);
            // Set the remote socket value to zero in the array, the order of these two lines of code cannot be changed.
            clientsocks[clientsocks[jj]] = 0;
            // Set the local socket value to zero in the array, the order of these two lines of code cannot be changed.
            clientsocks[jj] = 0;
          }
        }

        continue;
      }
      ////////////////////////////////////////////////////////

      ////////////////////////////////////////////////////////
      // If the event occurred on the listening socket listensock, it means that a new client has connected from the external network.
      int jj = 0;
      for (jj = 0; jj < vroute.size(); jj++)
      {
        if (evs[ii].data.fd == vroute[jj].listensock)
        {
          // Accept the connection from the external network client.
          struct sockaddr_in client;
          socklen_t len = sizeof(client);
          int srcsock = accept(vroute[jj].listensock, (struct sockaddr*)&client, &len);
          if (srcsock < 0)
            break;
          if (srcsock >= MAXSOCK)
          {
            logfile.Write("The number of connections has exceeded the maximum value %d.\n", MAXSOCK);
            close(srcsock);
            break;
          }

          // Send a command through the control channel to the internal network program, passing the routing parameters to it.
          char buffer[256];
          memset(buffer, 0, sizeof(buffer));
          sprintf(buffer, "<dstip>%s</dstip><dstport>%d</dstport>", vroute[jj].dstip, vroute[jj].dstport);
          if (send(cmdconnsock, buffer, strlen(buffer), 0) <= 0)
          {
            logfile.Write("Control channel with the internal network has been disconnected.\n");
            EXIT(-1);
          }

          // Accept the connection from the internal network program.
          int dstsock = accept(cmdlistensock, (struct sockaddr*)&client, &len);
          if (dstsock < 0)
          {
            close(srcsock);
            break;
          }
          if (dstsock >= MAXSOCK)
          {
            logfile.Write("The number of connections has exceeded the maximum value %d.\n", MAXSOCK);
            close(srcsock);
            close(dstsock);
            break;
          }

          // Connect the internal and external network client sockets together.

          // Prepare readable events for the two newly connected sockets and add them to epoll.
          ev.data.fd = srcsock;
          ev.events = EPOLLIN;
          epoll_ctl(epollfd, EPOLL_CTL_ADD, srcsock, &ev);
          ev.data.fd = dstsock;
          ev.events = EPOLLIN;
          epoll_ctl(epollfd, EPOLL_CTL_ADD, dstsock, &ev);

          // Update the values and active time of the two sockets in the clientsocks array.
          clientsocks[srcsock] = dstsock;
          clientsocks[dstsock] = srcsock;
          clientatime[srcsock] = time(0);
          clientatime[dstsock] = time(0);

          logfile.Write("Accepted port %d client (%d,%d) successfully.\n", vroute[jj].listenport, srcsock, dstsock);

          break;
        }
      }

      // If jj < vroute.size(), it means the event has been processed in the above for loop.
      if (jj < vroute.size())
        continue;
      ////////////////////////////////////////////////////////

      ////////////////////////////////////////////////////////
      // The following flow handles the events of the internal and external network communication link sockets.

      char buffer[5000]; // Data read from the socket.
      int buflen = 0;    // Size of the data read from the socket.

      // Read data from one end.
      memset(buffer, 0, sizeof(buffer));
      if ((buflen = recv(evs[ii].data.fd, buffer, sizeof(buffer), 0)) <= 0)
      {
        // If the connection has been disconnected, close both sockets.
        logfile.Write("Client (%d,%d) disconnected.\n", evs[ii].data.fd, clientsocks[evs[ii].data.fd]);
        close(evs[ii].data.fd);                    // Close the client's connection.
        close(clientsocks[evs[ii].data.fd]);       // Close the client's remote connection.
        clientsocks[clientsocks[evs[ii].data.fd]] = 0; // The order of these two lines of code cannot be changed.
        clientsocks[evs[ii].data.fd] = 0;             // The order of these two lines of code cannot be changed.

        continue;
      }

      // If data is successfully read, send the received message directly to the remote end.
      // logfile.Write("From %d to %d, %d bytes.\n", evs[ii].data.fd, clientsocks[evs[ii].data.fd], buflen);
      send(clientsocks[evs[ii].data.fd], buffer, buflen, 0);

      // Update the active time of both socket connections.
      clientatime[evs[ii].data.fd] = time(0);
      clientatime[clientsocks[evs[ii].data.fd]] = time(0);
    }
  }

  return 0;
}


// Initialize the server's listening port.
int initserver(int port)
{
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
  {
    perror("socket() failed");
    return -1;
  }

  int opt = 1;
  unsigned int len = sizeof(opt);
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, len);

  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  if (bind(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
  {
    perror("bind() failed");
    close(sock);
    return -1;
  }

  if (listen(sock, 5) != 0)
  {
    perror("listen() failed");
    close(sock);
    return -1;
  }

  return sock;
}

// Load proxy route parameters into the vroute container.
bool loadroute(const char* inifile)
{
  CFile File;

  if (File.Open(inifile, "r") == false)
  {
    logfile.Write("Failed to open the proxy route parameter file (%s).\n", inifile);
    return false;
  }

  char strBuffer[256];
  CCmdStr CmdStr;

  while (true)
  {
    memset(strBuffer, 0, sizeof(strBuffer));

    if (File.FFGETS(strBuffer, 200) == false)
      break;
    char* pos = strstr(strBuffer, "#");
    if (pos != 0)
      pos[0] = 0; // Delete comments.
    DeleteRChar(strBuffer, ' ');              // Delete right-side spaces.
    UpdateStr(strBuffer, "  ", " ", true);    // Replace two spaces with one space, note the third parameter.
    CmdStr.SplitToCmd(strBuffer, " ");
    if (CmdStr.CmdCount() != 3)
      continue;

    memset(&stroute, 0, sizeof(struct st_route));
    CmdStr.GetValue(0, &stroute.listenport);
    CmdStr.GetValue(1, stroute.dstip);
    CmdStr.GetValue(2, &stroute.dstport);

    vroute.push_back(stroute);
  }

  return true;
}

void EXIT(int sig)
{
  logfile.Write("Program exits, sig=%d.\n\n", sig);

  // Close the socket listening to internal network programs.
  close(cmdlistensock);

  // Close the control channel between the internal network program and the server.
  close(cmdconnsock);

  // Close all listening sockets.
  for (int ii = 0; ii < vroute.size(); ii++)
    close(vroute[ii].listensock);

  // Close all client sockets.
  for (int ii = 0; ii < MAXSOCK; ii++)
    if (clientsocks[ii] > 0)
      close(clientsocks[ii]);

  close(epollfd); // Close epoll.

  close(tfd); // Close the timer.

  exit(0);
}
