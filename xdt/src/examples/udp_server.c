/* local_dgram_server.c
 *
 * gcc -pedantic -W -Wall -o udp_server udp_server.c [-lsocket] [-lnsl]
 *
 * Simple UDP server receiving and printing datagrams from client counterparts.
 *
 * usage: ./local_dgram_server <port> [<address>]
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int
main(int argc, char **argv)
{
  int sockfd;
  struct sockaddr_in server, client;
  int port;
  char *address;
  int i;

  if (argc < 2) {
    fprintf(stderr, "usage: %s <port> [<address>]\n", argv[0]);
    exit(1);
  }
  port = atoi(argv[1]);
  address = (argc > 2) ? argv[2] : 0;

  /* create socket */
  if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    exit(10);
  }

  /* bind socket */
  memset(&server, 0, sizeof server);
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  if (!address) {
    server.sin_addr.s_addr = htonl(INADDR_ANY);
  } else if ((i=inet_pton(AF_INET, address, &server.sin_addr))<1) {
    fprintf(stderr, "inet_pton: %s\n", (i==-1)?strerror(errno):
	    "address does not contain a character string representing a valid IPv4 address");
    exit(15);
  }
  if (bind(sockfd, (struct sockaddr *)&server, sizeof server) == -1) {
    perror("bind");
    exit(20);
  }

  /* receive and print datagrams */
  for (;;) {
    char data[100];
    char peer[INET_ADDRSTRLEN];
    ssize_t bytes_read;
    socklen_t client_len = sizeof client;

    if ((bytes_read = recvfrom(sockfd, data, sizeof data - 1, 0, (struct sockaddr *)&client, &client_len)) == -1) {
      if (errno == EINTR) {
        break;
      }
      perror("recvfrom");
      exit(30);
    }

    if (!inet_ntop(AF_INET, &client.sin_addr, peer, sizeof peer)) {
      perror("inet_ntop");
      exit(40);
    }

    data[bytes_read] = 0;
    printf("%s: %s\n", peer, data);
  }

  return 0;
}
