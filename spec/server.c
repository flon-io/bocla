
//
// spec helper for bocla
//
// Sun Sep  7 19:27:55 JST 2014
//

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#include "server.h"


int server_ready = 0;
pid_t server_pid = -1;

void handle_usr1(int signal)
{
  printf(". server ready\n");
  server_ready = 1;
}


void serve()
{
  if (server_pid > 0) return;

  server_pid = fork();

  printf("############# pid: %i\n", server_pid);

  if (server_pid == 0)
  {
    execl("/bin/sh", "", "../spec/server.sh", NULL);

    perror("execl failed"); exit(1);
  }
  else
  {
    struct sigaction a;
    a.sa_handler = &handle_usr1;
    int r = sigaction(SIGUSR1, &a, NULL);

    if (r < 0) { perror("error setting USR1 handler"); exit(1); }

    while (1)
    {
      sleep(1); if (server_ready) { sleep(1); break; }
    }
  }
}

void unserve()
{
  if (server_pid < 1) return;

  // TODO: signal() server_pid
}

