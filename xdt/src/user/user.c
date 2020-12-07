/**
 * @file user.c
 * @ingroup user
 * @brief User layer runtime environment
 */

/**
 * @addtogroup user
 * @{
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "user.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <signal.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#ifndef SUN_LEN
#define SUN_LEN(sun) sizeof(*(sun))
#endif

/** 
 * @brief Zeroize an object
 *
 * @param buf object to fill with zero bytes
 */
#define ZERO(buf) memset(&(buf), 0, sizeof(buf))


/** @brief Unix domain socket for delivering SDU messages to the XDT layer */
static int send_sock = -1;

/** @brief Unix domain socket for receiving SDU messages from the XDT layer */
static int recv_sock = -1;

/** @brief Stores the socket address of #recv_sock */
static struct sockaddr_un recv_addr;

/** @brief Flag indicating, if the path stored in #recv_addr can be removed */
static volatile sig_atomic_t remove_sun_path = 0;


/**
 * @brief Exit handler
 *
 * Removes the path associated with the receiving 
 * unix domain socket on program termination.
 */
static void
cleanup_user(void)
{
  if (remove_sun_path) {
    remove(recv_addr.sun_path);
  }
}

/**
 * @brief Signal handler
 *
 * Terminates the process when SIGINT and SIGTERM is catched.
 */
static void
user_signal_handler(int signo)
{
  exit(0);

  /* surpress 'unused parameter' compiler warning */
  signo = signo;
}



/*** PUBLIC ***************************************************************/



/**
 * @brief Sets up the user instance
 *
 * After registering the exit handler, signal handlers for catching
 * SIGINT and SIGTERM are set.
 * If to set up a producer instance, a random bound unix domain socket
 * connected to the XDT layer is created.
 * An unix domain socket is bound to the path resolved by 
 * xdt_address_to_uap_name(@a ap) to receive SDU messages from the XDT layer.
 *
 * @param local address of the local access point
 * @param producer 0 if to setup a consumer, not 0 if to setup a producer
 */
void
setup_user(XDT_address * local, int producer)
{
  struct sigaction sa;
  struct sockaddr_un send_addr;

  assert(local);

  recv_addr.sun_path[0] = 0;

  /* register exit callback */
  if (atexit(cleanup_user) != 0) {
    fputs("setup_user: atexit() failed\n", stderr);
    exit(EXIT_FAILURE);
  }

  /* catch SIGINT and SIGTERM */
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
#ifdef SA_INTERRUPT
  sa.sa_flags |= SA_INTERRUPT;
#endif
  sa.sa_handler = user_signal_handler;

  if (sigaction(SIGINT, &sa, 0) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
  if (sigaction(SIGTERM, &sa, 0) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

  if (producer) {
    /* create random bound with service connected socket for sending */
    if ((send_sock = socket(PF_LOCAL, SOCK_DGRAM, 0)) == -1) {
      perror("setup_user: socket");
      exit(EXIT_FAILURE);
    }
    ZERO(send_addr);
    send_addr.sun_family = AF_LOCAL;
    if (xdt_address_to_sap_name(local, send_addr.sun_path, sizeof send_addr.sun_path) < 0) {
      fputs("setup_user: xdt_address_to_sap_name() failed\n", stderr);
      exit(EXIT_FAILURE);
    }
    if (connect(send_sock, (struct sockaddr *)&send_addr, SUN_LEN(&send_addr))
        == -1) {
      perror("setup_user: connect");
      exit(EXIT_FAILURE);
    }
  }

  /* create bound socket for receiving */
  if ((recv_sock = socket(PF_LOCAL, SOCK_DGRAM, 0)) == -1) {
    perror("setup_user: socket");
    exit(EXIT_FAILURE);
  }
  ZERO(recv_addr);
  recv_addr.sun_family = AF_LOCAL;
  if (xdt_address_to_uap_name(local, recv_addr.sun_path, sizeof recv_addr.sun_path) < 0) {
    fputs("setup_user: xdt_address_to_uap_name() failed\n", stderr);
    exit(EXIT_FAILURE);
  }
  if (bind(recv_sock, (struct sockaddr *)&recv_addr, SUN_LEN(&recv_addr)) == -1) {
    perror("setup_user: bind");
    fprintf(stderr, "Possible reasons:\n- another process is running using the same address\n- a previous run exited unclean, try to remove '%s'\n", recv_addr.sun_path);
    exit(EXIT_FAILURE);
  }
  remove_sun_path = 1;
}

/**
 * @brief Receives an SDU message from the XDT layer
 *
 * @param sdu points to the SDU message to be filled
 */
void
get_sdu(XDT_sdu * sdu)
{
  ssize_t bytes;

  if (!sdu) {
    fputs("get_sdu: null pointer as SDU argument\n", stderr);
    exit(EXIT_FAILURE);
  }

  if ((bytes = read(recv_sock, sdu, sizeof *sdu)) == -1) {
    perror("get_sdu: read");
    exit(EXIT_FAILURE);
  }

  if ((size_t) bytes < sizeof *sdu) {
    /* message to small */
    sdu->type = 0;
  }

  print_sdu(sdu, "received", stderr);
}

/**
 * @brief Delivers an SDU message to the XDT layer
 *
 * @param sdu points to the SDU message to deliver
 */
void
deliver_sdu(XDT_sdu * sdu)
{
  ssize_t bytes;

  if (!sdu) {
    fputs("send_sdu: null pointer as SDU argument\n", stderr);
    exit(EXIT_FAILURE);
  }

  print_sdu(sdu, "to send", stderr);

  if ((bytes = write(send_sock, sdu, sizeof *sdu)) == -1) {
    perror("send_sdu: write");
    exit(EXIT_FAILURE);
  }

  if ((size_t) bytes < sizeof *sdu) {
    fputs("send_sdu: could not send entire SDU\n", stderr);
    exit(EXIT_FAILURE);
  }
}

/**
 * @brief Reads payload data from @e stdin
 *
 * Only used in producer instances.
 *
 * @param buffer buffer to store the read piece of payload
 *
 * @return Number of bytes stored in @a buffer. If end of input is reached,
 *         a value < #XDT_DATA_MAX is returned.
 */
unsigned
read_data(char buffer[XDT_DATA_MAX])
{
  size_t bytes_read;
  size_t bytes_available = XDT_DATA_MAX;
  char *buf = buffer;

  do {
    if ((bytes_read = fread(buf, 1, bytes_available, stdin)) == 0) {
      if (ferror(stdin)) {
        fputs("read_data: fread() failed\n", stderr);
        exit(EXIT_FAILURE);
      }
    }

    bytes_available -= bytes_read;
    buf += bytes_read;
  } while (bytes_available && (bytes_read || !feof(stdin)));

  return buf - buffer;
}

/**
 * @brief Writes payload data to @e stdout
 *
 * Only used in consumer instances.
 *
 * @param buffer buffer containing the payload to write
 * @param length number of bytes to write
 */
void
write_data(char buffer[XDT_DATA_MAX], unsigned length)
{

  size_t bytes_written;
  size_t bytes_available = length;
  char *buf = buffer;

  if (length > XDT_DATA_MAX) {
    fputs("write_data: could not write SDU data (invalid length parameter)\n", stderr);
    exit(EXIT_FAILURE);
  }

  do {
    if ((bytes_written = fwrite(buf, 1, bytes_available, stdout)) == 0) {
      if (ferror(stdout)) {
        fputs("write_data: fwrite() failed\n", stderr);
        exit(EXIT_FAILURE);
      }
    }

    bytes_available -= bytes_written;
    buf += bytes_written;
  } while (bytes_available && (bytes_written || !feof(stdout)));
}


/**
 * @}
 */
