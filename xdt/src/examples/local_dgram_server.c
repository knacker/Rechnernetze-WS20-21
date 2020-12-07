/* local_dgram_server.c
 *
 * gcc -pedantic -W -Wall -o local_dgram_server local_dgram_server.c [-lsocket] [-lnsl]
 *
 * Simple Unix Domain Socket server receiving and printing (maybe cropped) datagrams
 * from client counterparts.
 *
 * usage: ./local_dgram_server
 */

#include "local_dgram.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>


static volatile sig_atomic_t should_quit = 0;

static void
signal_handler(int signo)
{
  should_quit = signo;
}


int
main(void)
{
  int sockfd;
  struct sigaction act;
  struct sockaddr_un server, client;

  /* catch SIGINT, so we can remove the unix path on interrupt */
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  act.sa_handler = signal_handler;
  if (sigaction(SIGINT, &act, 0) == -1) {
    perror("sigaction");
    exit(15);
  }

  /* create local socket */
  if ((sockfd = socket(PF_LOCAL, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    exit(10);
  }

  /* bind socket */
  memset(&server, 0, sizeof server);
  server.sun_family = AF_LOCAL;
  strncpy(server.sun_path, SERVER_PATH, sizeof server.sun_path - 1);
  if (bind(sockfd, (struct sockaddr *)&server, SUN_LEN(&server)) == -1) {
    perror("bind");
    exit(20);
  }

  /* receive and print datagrams */
  while (!should_quit) {
    char data[100];
    ssize_t bytes_read;
    socklen_t client_len = sizeof client;

    if ((bytes_read = recvfrom(sockfd, data, sizeof data - 1, 0, (struct sockaddr *)&client, &client_len)) == -1) {
      if (errno == EINTR) {
        break;
      }
      perror("recvfrom");
      exit(30);
    }

    data[bytes_read] = 0;
    printf("%s: %s\n", client_len ? client.sun_path : "<unknown>", data);
  }

  remove(SERVER_PATH);

  return 0;
}
