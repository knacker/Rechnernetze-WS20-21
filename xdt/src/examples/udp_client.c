/* local_dgram_server.c
 *
 * gcc -pedantic -W -Wall -o udp_client udp_client.c [-lsocket] [-lnsl]
 *
 * Simple UDP client sending a string to the server counterpart.
 * If (any) third argument given, the sending socket will be connected.
 *
 * usage: ./local_dgram_server <port> <address> [<connected>]
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define DATA "Hello from client."


int
main(int argc, char **argv)
{
  int sockfd;
  struct sockaddr_in server;
  int port;
  char *address;
  int connected = argc > 3;
  int i;

  if (argc < 3) {
    fprintf(stderr, "usage: %s <port> <address> [<connected>]\n", argv[0]);
    exit(1);
  }
  port = atoi(argv[1]);
  address = argv[2];

  /* create socket */
  if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    exit(10);
  }

  /* fill server address */
  memset(&server, 0, sizeof server);
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  if ((i=inet_pton(AF_INET, address, &server.sin_addr))<1) {
    fprintf(stderr, "inet_pton: %s\n", (i==-1)?strerror(errno):
	    "address does not contain a character string representing a valid IPv4 address");
    exit(15);
  }
  
  if (connected) {

    /* connect socket with server */
    if (connect(sockfd, (struct sockaddr *)&server, sizeof server) == -1) {
      perror("connect");
      exit(20);
    }

    if (write(sockfd, DATA, sizeof DATA) == -1) {
      perror("write");
      exit(30);
    }

  } else {

    if (sendto(sockfd, DATA, sizeof DATA, 0, (struct sockaddr *)&server, sizeof server) == -1) {
      perror("sendto");
      exit(40);
    }

  }

  return 0;
}
