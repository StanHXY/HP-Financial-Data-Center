/*
 * Program Name: inetd.cpp, Network Proxy Service Program.
*/
#include "_public.h"

// Structure for proxy route parameters.
struct st_route
{
  int  listenport;      // Local listening communication port.
  char dstip[31];       // Destination host's IP address.
  int  dstport;         // Destination host's communication port.
  int  listensock;      // Local listening socket.
} stroute;
vector<struct st_route> vroute;       // Container for proxy routes.
bool loadroute(const char *inifile);  // Load proxy route parameters into the vroute container.

// Initialize the server's listening port.
int initserver(int port);

int epollfd = 0;  // Epoll handle.
int tfd = 0;      // Timer handle.

#define MAXSOCK  1024
int clientsocks[MAXSOCK];       // Store the value of the socket at the other end of each socket connection.
int clientatime[MAXSOCK];       // Store the timestamp of the last send/receive message for each socket.

// Initiate a socket connection to the target IP and port.
int conntodst(const char *ip, const int port);

void EXIT(int sig);   // Process exit function.

CLogFile logfile;

CPActive PActive;     // Process heartbeat.

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    printf("\n");
    printf("Usage: ./inetd logfile inifile\n\n");
    printf("Sample: ./inetd /tmp/inetd.log /etc/inetd.conf\n\n");
    printf("        /project/tools1/bin/procctl 5 /project/tools1/bin/inetd /tmp/inetd.log /etc/inetd.conf\n\n");
    return -1;
  }

  // Close all signals and I/O.
  // Set signals, in the shell, you can use "kill + process number" to terminate these processes normally.
  // But please do not use "kill -9 + process number" to force termination.
  CloseIOAndSignal();
  signal(SIGINT, EXIT);
  signal(SIGTERM, EXIT);

  // Open the log file.
  if (logfile.Open(argv[1], "a+") == false)
  {
    printf("Failed to open the log file (%s).\n", argv[1]);
    return -1;
  }

  PActive.AddPInfo(30, "inetd");       // Set the process heartbeat timeout to 30 seconds.

  // Load proxy route parameters into the vroute container.
  if (loadroute(argv[2]) == false)
    return -1;

  logfile.Write("Loaded proxy route parameters successfully (%d).\n", vroute.size());

  // Initialize the server's listening sockets.
  for (int ii = 0; ii < vroute.size(); ii++)
  {
    if ((vroute[ii].listensock = initserver(vroute[ii].listenport)) < 0)
    {
      logfile.Write("initserver(%d) failed.\n", vroute[ii].listenport);
      EXIT(-1);
    }

    // Set the listening socket to non-blocking.
    fcntl(vroute[ii].listensock, F_SETFL, fcntl(vroute[ii].listensock, F_GETFD, 0) | O_NONBLOCK);
  }

  // Create the epoll handle.
  epollfd = epoll_create(1);

  struct epoll_event ev;  // Declare the event data structure.

  // Prepare the read events for the listening sockets.
  for (int ii = 0; ii < vroute.size(); ii++)
  {
    ev.events = EPOLLIN;                 // Read event.
    ev.data.fd = vroute[ii].listensock;  // Specify the custom data for the event, it will be returned together with the events by epoll_wait().
    epoll_ctl(epollfd, EPOLL_CTL_ADD, vroute[ii].listensock, &ev); // Add the listening socket event to epollfd.
  }

  // Create the timer.
  tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);  // Create the timerfd.

  struct itimerspec timeout;
  memset(&timeout, 0, sizeof(struct itimerspec));
  timeout.it_value.tv_sec = 20;   // Set the timeout to 20 seconds.
  timeout.it_value.tv_nsec = 0;
  timerfd_settime(tfd, 0, &timeout, NULL);  // Set the timer.

  // Prepare the event for the timer.
  ev.events = EPOLLIN | EPOLLET;      // Read event, note that it must be in ET mode.
  ev.data.fd = tfd;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, tfd, &ev);

  struct epoll_event evs[10];      // Store the events returned by epoll.

  while (true)
  {
    // Wait for events on the monitored sockets.
    int infds = epoll_wait(epollfd, evs, 10, -1);

    // Failed to return.
    if (infds < 0) {
      logfile.Write("epoll() failed.\n");
      break;
    }

    // Traverse the array evs returned by epoll.
    for (int ii = 0; ii < infds; ii++)
    {
      // logfile.Write("events=%d,data.fd=%d\n",evs[ii].events,evs[ii].data.fd);

      ////////////////////////////////////////////////////////
      // If the timer has expired, set the process heartbeat and clean up idle client sockets.
      if (evs[ii].data.fd == tfd)
      {
        timerfd_settime(tfd, 0, &timeout, NULL);  // Reset the timer.

        PActive.UptATime();        // Update the process heartbeat.

        for (int jj = 0; jj < MAXSOCK; jj++)
        {
          // If the client socket has been idle for more than 80 seconds, close it.
          if ((clientsocks[jj] > 0) && ((time(0) - clientatime[jj]) > 80))
          {
            logfile.Write("client(%d,%d) timeout.\n", clientsocks[jj], clientsocks[clientsocks[jj]]);
            close(clientsocks[jj]);  close(clientsocks[clientsocks[jj]]);
            // Set the socket value of the other end in the array to zero, these two lines of code cannot be reversed.
            clientsocks[clientsocks[jj]] = 0;
            // Set the socket value of this end in the array to zero, these two lines of code cannot be reversed.
            clientsocks[jj] = 0;
          }
        }

        continue;
      }
      ////////////////////////////////////////////////////////

      ////////////////////////////////////////////////////////
      // If the event is on the listensock, it means there is a new client connected.
      int jj = 0;
      for (jj = 0; jj < vroute.size(); jj++)
      {
        if (evs[ii].data.fd == vroute[jj].listensock)
        {
          // Accept the client's connection.
          struct sockaddr_in client;
          socklen_t len = sizeof(client);
          int srcsock = accept(vroute[jj].listensock, (struct sockaddr*)&client, &len);
          if (srcsock < 0) break;
          if (srcsock >= MAXSOCK)
          {
            logfile.Write("The number of connections has exceeded the maximum value %d.\n", MAXSOCK);
            close(srcsock); break;
          }

          // Initiate a socket connection to the target IP and port.
          int dstsock = conntodst(vroute[jj].dstip, vroute[jj].dstport);
          if (dstsock < 0) break;
          if (dstsock >= MAXSOCK)
          {
            logfile.Write("The number of connections has exceeded the maximum value %d.\n", MAXSOCK);
            close(srcsock); close(dstsock); break;
          }

          logfile.Write("Accept on port %d client(%d,%d) ok.\n", vroute[jj].listenport, srcsock, dstsock);

          // Prepare read events for the two newly connected sockets and add them to epoll.
          ev.data.fd = srcsock; ev.events = EPOLLIN;
          epoll_ctl(epollfd, EPOLL_CTL_ADD, srcsock, &ev);
          ev.data.fd = dstsock; ev.events = EPOLLIN;
          epoll_ctl(epollfd, EPOLL_CTL_ADD, dstsock, &ev);

          // Update the socket values and active time in the clientsocks array for the two ends of the new connection.
          clientsocks[srcsock] = dstsock; clientsocks[dstsock] = srcsock;
          clientatime[srcsock] = time(0); clientatime[dstsock] = time(0);

          break;
        }
      }

      // If jj < vroute.size(), it means the event has been handled in the above for loop.
      if (jj < vroute.size()) continue;
      ////////////////////////////////////////////////////////

      ////////////////////////////////////////////////////////
      // If there is an event on a client connection socket, it means there is data sent or the connection is disconnected.

      char buffer[5000];   // Store the data read from the client.
      int  buflen = 0;       // Size of the data read from the socket.

      // Read data from one end.
      memset(buffer, 0, sizeof(buffer));
      if ((buflen = recv(evs[ii].data.fd, buffer, sizeof(buffer), 0)) <= 0)
      {
        // If the connection is disconnected, we need to close both sockets.
        logfile.Write("Client(%d,%d) disconnected.\n", evs[ii].data.fd, clientsocks[evs[ii].data.fd]);
        close(evs[ii].data.fd);                            // Close the client's connection.
        close(clientsocks[evs[ii].data.fd]);               // Close the other end of the client's connection.
        clientsocks[clientsocks[evs[ii].data.fd]] = 0;       // These two lines of code cannot be reversed.
        clientsocks[evs[ii].data.fd] = 0;                    // These two lines of code cannot be reversed.

        continue;
      }

      // Successfully read data, send the received content back to the other end without modification.
      // logfile.Write("From %d to %d, %d bytes.\n", evs[ii].data.fd, clientsocks[evs[ii].data.fd], buflen);
      send(clientsocks[evs[ii].data.fd], buffer, buflen, 0);

      // Update the last active time of the client connection.
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

  if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
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
bool loadroute(const char *inifile)
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
    char *pos = strstr(strBuffer, "#");
    if (pos != 0)
      pos[0] = 0;                  // Remove comment.
    DeleteRChar(strBuffer, ' ');            // Remove right spaces.
    UpdateStr(strBuffer, "  ", " ", true);    // Replace two spaces with one space.
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

// Initiate a socket connection to the target IP and port.
int conntodst(const char *ip, const int port)
{
  // Step 1: Create the client's socket.
  int sockfd;
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    return -1;

  // Step 2: Send a connection request to the server.
  struct hostent *h;
  if ((h = gethostbyname(ip)) == 0)
  {
    close(sockfd);
    return -1;
  }

  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port); // Specify the server's communication port.
  memcpy(&servaddr.sin_addr, h->h_addr, h->h_length);

  // Set the socket to non-blocking.
  fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK);

  connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

  return sockfd;
}

void EXIT(int sig)
{
  logfile.Write("Program exit, sig=%d.\n\n", sig);

  // Close all listening sockets.
  for (int ii = 0; ii < vroute.size(); ii++)
    close(vroute[ii].listensock);

  // Close all client sockets.
  for (int ii = 0; ii < MAXSOCK; ii++)
    if (clientsocks[ii] > 0)
      close(clientsocks[ii]);

  close(epollfd);   // Close epoll.

  close(tfd);       // Close the timer.

  exit(0);
}
