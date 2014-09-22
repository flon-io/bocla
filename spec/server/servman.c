
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
#include <string.h>
#include <sys/types.h>

#include "servman.h"


pid_t server_pid = -1;


void server_start()
{
  if (server_pid > 0) return;

  server_pid = fork();

  if (server_pid == 0)
  {
    char *v = getenv("VALSERV");

    if (v != NULL && (strcmp(v, "1") == 0 || strcmp(v, "true") == 0))
    {
      //char *env[] = { "FGAJ_HOST=g", "FGAJ_LEVEL=10", NULL };
      char *env[] = { "FGAJ_HOST=g", NULL };
      execle(
        "/usr/bin/valgrind", "",
        "../spec/server/tmp/server", "", NULL, env);
    }
    else
    {
      char *env[] = { "FGAJ_HOST=g", "FGAJ_LEVEL=10", NULL };
      //char *env[] = { "FGAJ_HOST=g", NULL };
      execle("../spec/server/tmp/server", "", NULL, env);
    }

    perror("execle failed"); exit(1);
  }
  else
  {
    sleep(1); // wait for server to get ready
  }
}

void server_stop()
{
  if (server_pid < 1) return;

  printf("stopping %i...\n", server_pid);
  kill(server_pid, SIGTERM);

  sleep(1);
}

