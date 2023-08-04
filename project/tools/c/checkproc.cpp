#include "_public.h"

// Program log.
CLogFile logfile;

int main(int argc, char* argv[])
{
  // Program help.
  if (argc != 2)
  {
    printf("\n");
    printf("Usage: ./checkproc logfilename\n");
    printf("Example: /project/tools1/bin/procctl 10 /project/tools1/bin/checkproc /tmp/log/checkproc.log\n\n");
    printf("This program is used to check whether background service programs have timed out and terminate them if they have timed out.\n");
    printf("Note:\n");
    printf("1) This program should be launched by procctl with a running cycle of 10 seconds.\n");
    printf("2) To avoid being mistakenly terminated by regular users, this program should be launched by the root user.\n");
    printf("3) To stop this program, use 'killall -9' to terminate it.\n\n\n");
    return 0;
  }

  // Ignore all signals and I/O to prevent program interruptions.
  CloseIOAndSignal(true);

  // Open the log file.
  if (logfile.Open(argv[1], "a+") == false)
  {
    printf("logfile.Open(%s) failed.\n", argv[1]);
    return -1;
  }

  int shmid = 0;

  // Create/get shared memory, the key value is SHMKEYP, and the size is the size of MAXNUMP st_procinfo structures.
  if ((shmid = shmget((key_t)SHMKEYP, MAXNUMP * sizeof(struct st_procinfo), 0666 | IPC_CREAT)) == -1)
  {
    logfile.Write("Create/get shared memory(%x) failed.\n", SHMKEYP);
    return false;
  }

  // Attach the shared memory to the current process's address space.
  struct st_procinfo* shm = (struct st_procinfo*)shmat(shmid, 0, 0);

  // Traverse all records in the shared memory.
  for (int ii = 0; ii < MAXNUMP; ii++)
  {
    // If the pid of the record is 0, it means it is an empty record, continue;
    if (shm[ii].pid == 0)
      continue;

    // If the pid of the record is not 0, it means it is a heartbeat record of a service program.

    // After the program runs stably, the following two lines of code can be commented out.
    //logfile.Write("ii=%d,pid=%d,pname=%s,timeout=%d,atime=%d\n",\
    //               ii, shm[ii].pid, shm[ii].pname, shm[ii].timeout, shm[ii].atime);

    // Send signal 0 to the process to check if it still exists. If it does not exist, remove the record from the shared memory and continue;
    int iret = kill(shm[ii].pid, 0);
    if (iret == -1)
    {
      logfile.Write("Process pid=%d(%s) no longer exists.\n", (shm + ii)->pid, (shm + ii)->pname);
      memset(shm + ii, 0, sizeof(struct st_procinfo)); // Remove the record from the shared memory.
      continue;
    }

    time_t now = time(0); // Get the current time.

    // If the process has not timed out, continue;
    if (now - shm[ii].atime < shm[ii].timeout)
      continue;

    // If it has timed out.
    logfile.Write("Process pid=%d(%s) has timed out.\n", (shm + ii)->pid, (shm + ii)->pname);

    // Send signal 15 to attempt to terminate the process normally.
    kill(shm[ii].pid, 15);

    // Check if the process exists every 1 second, up to 5 seconds. Generally, 5 seconds are enough for the process to exit.
    for (int jj = 0; jj < 5; jj++)
    {
      sleep(1);
      iret = kill(shm[ii].pid, 0); // Send signal 0 to the process to check if it still exists.
      if (iret == -1)
        break; // The process has exited.
    }

    // If the process still exists, send signal 9 to force terminate it.
    if (iret == -1)
      logfile.Write("Process pid=%d(%s) has exited normally.\n", (shm + ii)->pid, (shm + ii)->pname);
    else
    {
      kill(shm[ii].pid, 9); // If the process still exists, send signal 9 to force terminate it.
      logfile.Write("Process pid=%d(%s) has been forcibly terminated.\n", (shm + ii)->pid, (shm + ii)->pname);
    }

    // Remove the heartbeat record of the timed-out process from the shared memory.
    memset(shm + ii, 0, sizeof(struct st_procinfo)); // Remove the record from the shared memory.
  }

  // Detach the shared memory from the current process.
  shmdt(shm);

  return 0;
}
