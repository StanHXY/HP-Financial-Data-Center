#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/epoll.h>

#define MAXEVENTS 10

// Initialize the server's listening port.
int initserver(int port);

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("usage: ./tcpepoll port\n");
    return -1;
  }

  // Initialize the socket for the server to listen on.
  int listensock = initserver(atoi(argv[1]));
  printf("listensock = %d\n", listensock);

  if (listensock < 0)
  {
    printf("initserver() failed.\n");
    return -1;
  }

  int epollfd = epoll_create(1); // Create the epoll handle.

  // Prepare the readable event for the listening socket.
  struct epoll_event ev; // Declare the event data structure.
  ev.events = EPOLLIN;   // Read event.
  ev.data.fd = listensock; // Specify the custom data for the event, it will be returned with the events by epoll_wait().

  // Add the event of the listening socket to epollfd.
  epoll_ctl(epollfd, EPOLL_CTL_ADD, listensock, &ev);

  while (true)
  {
    struct epoll_event events[MAXEVENTS]; // Store the events returned by epoll.

    // Wait for events on the monitored sockets.
    int infds = epoll_wait(epollfd, events, MAXEVENTS, -1);

    // Return failure.
    if (infds < 0)
    {
      perror("epoll() failed");
      break;
    }

    // Timeout.
    if (infds == 0)
    {
      printf("epoll() timeout.\n");
      continue;
    }

    // If infds > 0, it means there are events occurred.
    // Traverse the array of events returned by epoll.
    for (int ii = 0; ii < infds; ii++)
    {
      if ((events[ii].data.fd == listensock) && (events[ii].events & EPOLLIN))
      {
        // If the event is for listensock, it means a new client has connected.
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        int clientsock = accept(listensock, (struct sockaddr *)&client, &len);
        if (clientsock < 0)
        {
          printf("accept() failed.\n");
          continue;
        }

        // Prepare the readable event for the new client, and add it to epoll.
        memset(&ev, 0, sizeof(struct epoll_event));
        ev.data.fd = clientsock;
        ev.events = EPOLLIN;
        epoll_ctl(epollfd, EPOLL_CTL_ADD, clientsock, &ev);

        printf("accept client(socket = %d) ok.\n", clientsock);
      }
      else
      {
        // If the event is for a client socket, it means data has been sent or the connection has been closed.
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));

        // Read data from the client.
        ssize_t isize = read(events[ii].data.fd, buffer, sizeof(buffer));

        // Error occurred or the socket was closed by the other side.
        if (isize <= 0)
        {
          printf("client(eventfd = %d) disconnected.\n", events[ii].data.fd);

          close(events[ii].data.fd);
        }
        else
        {
          // If data has been received from the client.
          printf("recv(eventfd = %d, size = %ld): %s\n", events[ii].data.fd, isize, buffer);
          send(events[ii].data.fd, buffer, strlen(buffer), 0);
        }
      }
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


/*
EPOLLIN     // Indicates that the corresponding file descriptor is ready for reading (including normal socket closure by the other end).
EPOLLOUT    // Indicates that the corresponding file descriptor is ready for writing.
EPOLLPRI    // Indicates that the corresponding file descriptor has urgent data available for reading (usually refers to out-of-band data).
EPOLLERR    // Indicates that an error has occurred on the corresponding file descriptor.
EPOLLHUP    // Indicates that the corresponding file descriptor has been hung up (the other end has closed the connection).
EPOLLET     // Sets EPOLL to edge-triggered mode (compared to the default level-triggered mode).
EPOLLONESHOT // Only listen for the next occurrence of the event, after the event is handled, the socket needs to be added to the EPOLL queue again if you want to continue monitoring.
*/

