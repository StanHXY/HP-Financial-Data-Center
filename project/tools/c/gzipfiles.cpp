#include "_public.h"

// Function to handle program exit and signals 2 and 15.
void EXIT(int sig);

int main(int argc, char *argv[])
{
  // Program help.
  if (argc != 4)
  {
    printf("\n");
    printf("Usage: /project/tools1/bin/gzipfiles pathname matchstr timeout\n\n");

    printf("Example: /project/tools1/bin/gzipfiles /log/idc \"*.log.20*\" 0.02\n");
    printf("         /project/tools1/bin/gzipfiles /tmp/idc/surfdata \"*.xml,*.json\" 0.01\n");
    printf("         /project/tools1/bin/procctl 300 /project/tools1/bin/gzipfiles /log/idc \"*.log.20*\" 0.02\n");
    printf("         /project/tools1/bin/procctl 300 /project/tools1/bin/gzipfiles /tmp/idc/surfdata \"*.xml,*.json\" 0.01\n\n");

    printf("This is a utility program to compress historical data files or log files.\n");
    printf("The program compresses all files in the 'pathname' directory and its subdirectories that match the 'matchstr' pattern and are older than 'timeout' days.\n");
    printf("The 'timeout' can be a decimal number.\n");
    printf("This program does not write log files or output any information to the console.\n");
    printf("The program calls the /usr/bin/gzip command to compress the files.\n\n\n");

    return -1;
  }

  // Close all signals and I/O.
  // Set the signals to allow normal termination using "kill + process number" in the shell.
  // But please do not use "kill -9 + process number" to force termination.
  CloseIOAndSignal(true);
  signal(SIGINT, EXIT);
  signal(SIGTERM, EXIT);

  // Get the time point of file timeout.
  char strTimeOut[21];
  LocalTime(strTimeOut, "yyyy-mm-dd hh24:mi:ss", 0 - (int)(atof(argv[3]) * 24 * 60 * 60));

  CDir Dir;
  // Open the directory using CDir.OpenDir().
  if (!Dir.OpenDir(argv[1], argv[2], 10000, true))
  {
    printf("Dir.OpenDir(%s) failed.\n", argv[1]);
    return -1;
  }

  char strCmd[1024]; // Buffer to store the gzip command.

  // Traverse the file names in the directory.
  while (true)
  {
    // Get information about a file using CDir.ReadDir().
    if (!Dir.ReadDir())
      break;

    // Compare the modification time with the timeout. If it's older, compression is required.
    if ((strcmp(Dir.m_ModifyTime, strTimeOut) < 0) && (!MatchStr(Dir.m_FileName, "*.gz")))
    {
      // Compress the file using the operating system's gzip command.
      snprintf(strCmd, sizeof(strCmd), "/usr/bin/gzip -f %s 1>/dev/null 2>/dev/null", Dir.m_FullFileName);
      if (system(strCmd) == 0)
        printf("gzip %s ok.\n", Dir.m_FullFileName);
      else
        printf("gzip %s failed.\n", Dir.m_FullFileName);
    }
  }

  return 0;
}

void EXIT(int sig)
{
  printf("Program exited, sig=%d\n\n", sig);

  exit(0);
}
