#include "_public.h"

// Function to handle program exit and signals 2, 15.
void EXIT(int sig);

int main(int argc, char* argv[])
{
  // Program help.
  if (argc != 4)
  {
    printf("\n");
    printf("Usage: /project/tools1/bin/deletefiles pathname matchstr timeout\n\n");

    printf("Example: /project/tools1/bin/deletefiles /log/idc \"*.log.20*\" 0.02\n");
    printf("         /project/tools1/bin/deletefiles /tmp/idc/surfdata \"*.xml,*.json\" 0.01\n");
    printf("         /project/tools1/bin/procctl 300 /project/tools1/bin/deletefiles /log/idc \"*.log.20*\" 0.02\n");
    printf("         /project/tools1/bin/procctl 300 /project/tools1/bin/deletefiles /tmp/idc/surfdata \"*.xml,*.json\" 0.01\n\n");

    printf("This is a utility program used to delete historical data files or log files.\n");
    printf("This program deletes all files that match 'matchstr' and are older than 'timeout' days in the 'pathname' directory and its subdirectories.\n");
    printf("'timeout' can be a floating-point number.\n");
    printf("This program does not write log files and will not output any information to the console.\n\n\n");

    return -1;
  }

  // Close all signals and input/output.
  signal(SIGINT, EXIT);
  signal(SIGTERM, EXIT);

  // Get the timeout time point for files.
  char strTimeOut[21];
  LocalTime(strTimeOut, "yyyy-mm-dd hh24:mi:ss", 0 - (int)(atof(argv[3]) * 24 * 60 * 60));

  CDir Dir;
  // Open the directory, CDir.OpenDir()
  if (Dir.OpenDir(argv[1], argv[2], 10000, true) == false)
  {
    printf("Dir.OpenDir(%s) failed.\n", argv[1]);
    return -1;
  }

  // Traverse the file names in the directory.
  while (true)
  {
    // Get information about a file, CDir.ReadDir()
    if (Dir.ReadDir() == false)
      break;
    printf("=%s=\n", Dir.m_FullFileName);
    // Compare with the timeout time point, if it's earlier, then it needs to be deleted.
    if (strcmp(Dir.m_ModifyTime, strTimeOut) < 0)
    {
      if (REMOVE(Dir.m_FullFileName) == 0)
        printf("REMOVE %s ok.\n", Dir.m_FullFileName);
      else
        printf("REMOVE %s failed.\n", Dir.m_FullFileName);
    }
  }

  return 0;
}

void EXIT(int sig)
{
  printf("Program exits, sig=%d\n\n", sig);

  exit(0);
}
