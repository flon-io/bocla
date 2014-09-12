
//
// spec helper for bocla
//
// Sun Sep  7 19:27:55 JST 2014
//

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

#include "servman.h"


pid_t server_pid = -1;


void server_start()
{
  if (server_pid > 0) return;

  server_pid = fork();

  if (server_pid == 0)
  {
    execl("../spec/server/tmp/server", "", NULL);

    perror("execl failed"); exit(1);
  }
  else
  {
    sleep(2); // wait for server to get ready
  }
}

void server_stop()
{
  if (server_pid < 1) return;

  printf("stopping %i...\n", server_pid);
  kill(server_pid, SIGTERM);

  sleep(1);
}

