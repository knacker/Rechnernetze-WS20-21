/* local_dgram_client.c
 *
 * gcc -pedantic -W -Wall -o local_dgram_client local_dgram_client.c [-lsocket] [-lnsl]
 *
 * Simple Unix Domain Socket client sending a string to the server counterpart.
 * If any command line arguments given, the sending socket will be bound.
 *
 * usage: ./local_dgram_client [<bound>]
 */

#include "local_dgram.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define CLIENT_PATH "/tmp/example_client_path"
#define DATA "Hello from client."


int
main(int argc, char **argv)
{
  int do_bind = argc > 1;
  int sockfd;
  struct sockaddr_un client, server;

  /* supress 'unused parameter' warning */
  argv = argv;

  /* create local socket */
  if ((sockfd = socket(PF_LOCAL, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    exit(10);
  }

  if (do_bind) {
    /* bind client socket to path */
    remove(CLIENT_PATH);
    memset(&client, 0, sizeof client);
    client.sun_family = AF_LOCAL;
    strncpy(client.sun_path, CLIENT_PATH, sizeof client.sun_path - 1);
    if (bind(sockfd, (struct sockaddr *)&client, SUN_LEN(&client)) == -1) {
      perror("bind");
      exit(20);
    }
  }

  /* fill server address */
  memset(&server, 0, sizeof server);
  server.sun_family = AF_LOCAL;
  strncpy(server.sun_path, SERVER_PATH, sizeof server.sun_path - 1);

  if (sendto(sockfd, DATA, sizeof DATA, 0, (struct sockaddr *)&server, SUN_LEN(&server)) == -1) {
    perror("sendto");
  }

  if (do_bind) {
    remove(client.sun_path);
  }

  return 0;
}
