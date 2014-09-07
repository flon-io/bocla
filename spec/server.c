
//
// spec helper for bocla
//
// Sun Sep  7 19:27:55 JST 2014
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "server.h"


pid_t server_pid = -1;


void serve()
{
  if (server_pid > 0) return;

  server_pid = fork();

  printf("############# pid: %i\n", server_pid);

  if (server_pid != 0) return;
  //if (server_pid != 0)
  //{
  //  int status;
  //  wait(&status);
  //  printf("child exited with status %i\n", status);
  //  exit(1);
  //}

  execl("/bin/sh", "", "../spec/server.sh", NULL);

  perror("execl failed"); exit(1);
}

void unserve()
{
  if (server_pid < 1) return;

  // TODO: signal() server_pid
}

