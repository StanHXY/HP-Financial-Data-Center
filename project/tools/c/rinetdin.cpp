/*
 * Program: rinetdin.cpp, Network Proxy Service Program - Internal Network End.
*/
#include "_public.h"

int cmdconnsock; // Control channel between the internal network program and the external network program.

int epollfd = 0; // epoll handle.
int tfd = 0;     // Timer handle.

#define MAXSOCK 1024
int clientsocks[MAXSOCK]; // Stores the value of each socket's connected peer socket.
int clientatime[MAXSOCK]; // Stores the last time each socket sent or received a message.

// Initiate a socket connection to the target IP and port.
int conntodst(const char* ip, const int port);

void EXIT(int sig); // Process exit function.

CLogFile logfile;

CPActive PActive; // Process heartbeat.

int main(int argc, char* argv[])
{
  if (argc != 4)
  {
    printf("\n");
    printf("Using :./rinetdin logfile ip port\n\n");
    printf("Sample:./rinetdin /tmp/rinetdin.log 192.168.174.132 4000\n\n");
    printf("        /project/tools1/bin/procctl 5 /project/tools1/bin/rinetdin /tmp/rinetdin.log 192.168.174.132 4000\n\n");
    printf("logfile This program's log file name.\n");
    printf("ip      External network proxy server address.\n");
    printf("port    External network proxy server port.\n\n\n");
    return -1;
  }

  // Close all signals and input/output.
  // Set up signals. You can use "kill + process number" to terminate the process normally in shell.
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

  PActive.AddPInfo(30, "inetd"); // Set the process heartbeat timeout to 30 seconds.

  // Establish a control channel between the internal network program and the external network program.
  CTcpClient TcpClient;
  if (TcpClient.ConnectToServer(argv[2], atoi(argv[3])) == false)
  {
    logfile.Write("TcpClient.ConnectToServer(%s,%s) failed.\n", argv[2], argv[3]);
    return -1;
  }

  cmdconnsock = TcpClient.m_connfd;
  fcntl(cmdconnsock, F_SETFL, fcntl(cmdconnsock, F_GETFD, 0) | O_NONBLOCK);

  logfile.Write("Control channel with the external network has been established (cmdconnsock=%d).\n", cmdconnsock);

  // Create epoll handle.
  epollfd = epoll_create(1);

  struct epoll_event ev; // Declare the event data structure.

  // Prepare readable events for the control channel's socket.
  ev.events = EPOLLIN;
  ev.data.fd = cmdconnsock;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, cmdconnsock, &ev);

  // Create a timer.
  tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC); // Create timerfd.

  struct itimerspec timeout;
  memset(&timeout, 0, sizeof(struct itimerspec));
  timeout.it_value.tv_sec = 20; // Timeout is set to 20 seconds.
  timeout.it_value.tv_nsec = 0;
  timerfd_settime(tfd, 0, &timeout, NULL); // Set the timer.

  // Prepare the event for the timer.
  ev.events = EPOLLIN | EPOLLET; // Read event, make sure to use ET mode.
  ev.data.fd = tfd;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, tfd, &ev);

  PActive.AddPInfo(30, "rinetdin"); // Set the process heartbeat timeout to 30 seconds.

  struct epoll_event evs[10]; // Store the events returned by epoll.

  while (true)
  {
    // Wait for events on the monitored sockets.
    int infds = epoll_wait(epollfd, evs, 10, -1);

    // Return failed.
    if (infds < 0)
    {
      logfile.Write("epoll() failed.\n");
      break;
    }

    // Traverse the array of events returned by epoll.
    for (int ii = 0; ii < infds; ii++)
    {
      ////////////////////////////////////////////////////////
      // If the timer has expired, update the process heartbeat and clean up idle client sockets.
      if (evs[ii].data.fd == tfd)
      {
        timerfd_settime(tfd, 0, &timeout, NULL); // Reset the timer.

        PActive.UptATime(); // Update process heartbeat.

        for (int jj = 0; jj < MAXSOCK; jj++)
        {
          // If the client socket has been idle for more than 80 seconds, close it.
          if ((clientsocks[jj] > 0) && ((time(0) - clientatime[jj]) > 80))
          {
            logfile.Write("client(%d,%d) timeout.\n", clientsocks[jj], clientsocks[clientsocks[jj]]);
            close(clientsocks[jj]);
            close(clientsocks[clientsocks[jj]]);
            // Set the peer socket in the array to empty. The order of the following two lines of code cannot be changed.
            clientsocks[clientsocks[jj]] = 0;
            // Set the local socket in the array to empty. The order of the following two lines of code cannot be changed.
            clientsocks[jj] = 0;
          }
        }

        continue;
      }
      ////////////////////////////////////////////////////////

      // If the event occurred on the control channel.
      if (evs[ii].data.fd == cmdconnsock)
      {
        // Read the control message content.
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        if (recv(cmdconnsock, buffer, 200, 0) <= 0)
        {
          logfile.Write("Control channel to the external network has been disconnected.\n");
          EXIT(-1);
        }

        // If it's a heartbeat message, continue.
        if (strcmp(buffer, "<activetest>") == 0)
          continue;

        // If it's a command to establish a new connection.

        // Initiate a connection request to the external network server.
        int srcsock = conntodst(argv[2], atoi(argv[3]));
        if (srcsock < 0)
          continue;
        if (srcsock >= MAXSOCK)
        {
          logfile.Write("Number of connections has exceeded the maximum value %d.\n", MAXSOCK);
          close(srcsock);
          continue;
        }

        // Get the target service address and port from the control message content.
        char dstip[11];
        int dstport;
        GetXMLBuffer(buffer, "dstip", dstip, 30);
        GetXMLBuffer(buffer, "dstport", &dstport);

        // Initiate a socket connection to the target service address and port.
        int dstsock = conntodst(dstip, dstport);
        if (dstsock < 0)
        {
          close(srcsock);
          continue;
        }
        if (dstsock >= MAXSOCK)
        {
          logfile.Write("Number of connections has exceeded the maximum value %d.\n", MAXSOCK);
          close(srcsock);
          close(dstsock);
          continue;
        }

        // Connect the internal and external network sockets together.
        logfile.Write("New internal and external network channel (%d,%d) established.\n", srcsock, dstsock);

        // Prepare readable events for the two newly connected sockets and add them to epoll.
        ev.data.fd = srcsock;
        ev.events = EPOLLIN;
        epoll_ctl(epollfd, EPOLL_CTL_ADD, srcsock, &ev);
        ev.data.fd = dstsock;
        ev.events = EPOLLIN;
        epoll_ctl(epollfd, EPOLL_CTL_ADD, dstsock, &ev);

        // Update the values and activity time of the two sockets in the clientsocks array.
        clientsocks[srcsock] = dstsock;
        clientsocks[dstsock] = srcsock;
        clientatime[srcsock] = time(0);
        clientatime[dstsock] = time(0);

        continue;
      }
      ////////////////////////////////////////////////////////

      ////////////////////////////////////////////////////////
      // The following process handles events for the internal and external network communication link sockets.

      char buffer[5000]; // Data read from the socket.
      int buflen = 0;    // Size of data read from the socket.

      // Read data from one end.
      memset(buffer, 0, sizeof(buffer));
      if ((buflen = recv(evs[ii].data.fd, buffer, sizeof(buffer), 0)) <= 0)
      {
        // If the connection is disconnected, close both sockets of the channel.
        logfile.Write("client(%d,%d) disconnected.\n", evs[ii].data.fd, clientsocks[evs[ii].data.fd]);
        close(evs[ii].data.fd);                           // Close the client's connection.
        close(clientsocks[evs[ii].data.fd]);              // Close the client's peer connection.
        clientsocks[clientsocks[evs[ii].data.fd]] = 0;    // The order of the following two lines of code cannot be changed.
        clientsocks[evs[ii].data.fd] = 0;                 // The order of the following two lines of code cannot be changed.

        continue;
      }

      // Successfully read the data, send the received message content as it is to the other end.
      // logfile.Write("from %d to %d,%d bytes.\n", evs[ii].data.fd, clientsocks[evs[ii].data.fd], buflen);
      send(clientsocks[evs[ii].data.fd], buffer, buflen, 0);

      // Update the activity time of both ends of the socket connection.
      clientatime[evs[ii].data.fd] = time(0);
      clientatime[clientsocks[evs[ii].data.fd]] = time(0);
    }
  }

  return 0;
}


// Initiate a socket connection to the target IP and port.
int conntodst(const char* ip, const int port)
{
  // Step 1: Create the client's socket.
  int sockfd;
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    return -1;

  // Step 2: Send a connection request to the server.
  struct hostent* h;
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

  // Set the socket as non-blocking.
  fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK);

  connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

  return sockfd;
}

void EXIT(int sig)
{
  logfile.Write("Program exit, sig=%d.\n\n", sig);

  // Close the control channel between the internal and external network programs.
  close(cmdconnsock);

  // Close all client sockets.
  for (int ii = 0; ii < MAXSOCK; ii++)
  {
    if (clientsocks[ii] > 0)
      close(clientsocks[ii]);
  }

  close(epollfd); // Close epoll.

  close(tfd); // Close the timer.

  exit(0);
}

