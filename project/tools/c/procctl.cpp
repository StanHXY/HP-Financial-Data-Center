#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    printf("Usage: ./procctl timetvl program argv ...\n");
    printf("Example: /project/tools1/bin/procctl 5 /usr/bin/tar zcvf /tmp/tmp.tgz /usr/include\n\n");

    printf("This program is the scheduler for service programs, it periodically starts service programs or shell scripts.\n");
    printf("timetvl: Run interval in seconds. After the scheduled program finishes running, it will be restarted by procctl after timetvl seconds.\n");
    printf("program: The name of the scheduled program, it must be the full path.\n");
    printf("argvs: The arguments for the scheduled program.\n");
    printf("Note that this program will not be killed by kill command, but it can be forcefully terminated using kill -9.\n\n\n");

    return -1;
  }

  // Close signals and IO, this program should not be disturbed.
  for (int ii = 0; ii < 64; ii++)
  {
    signal(ii, SIG_IGN);
    close(ii);
  }

  // Generate a child process, and the parent process exits, allowing the program to run in the background, supervised by system init process.
  if (fork() != 0)
    exit(0);

  // Enable SIGCHLD signal, so that the parent process can wait for the child process to exit.
  signal(SIGCHLD, SIG_DFL);

  char *pargv[argc];
  for (int ii = 2; ii < argc; ii++)
    pargv[ii - 2] = argv[ii];

  pargv[argc - 2] = NULL;

  while (true)
  {
    if (fork() == 0)
    {
      execv(argv[2], pargv);
      exit(0);
    }
    else
    {
      int status;
      wait(&status);
      sleep(atoi(argv[1]));
    }
  }
}
