/**
 * @file service/main.c
 * @ingroup service
 * @brief Service program entry point
 */

/** 
 * @defgroup service XDT Layer
 *
 * @brief XDT Layer implementation
 *
 * All sources in the @e service directory belong to this module. 
 *
 * This is the XDT layer implementation, it acts simultaneously 
 * as both a sender and receiver.
 * At startup the command line address is parsed and the optionally 
 * error case to simulate is evaluated.
 *
 *
 * The dispatch() function establishes listening UDP and Unix Domain Sockets.
 * When the dispatcher identifies a new connection, communication endpoints
 * for sending packets are established. A new sender or receiver
 * process is spawned and the appropriate entry function start_sender() or
 * start_receiver() is called. These instances receive messages from a
 * message queue, which is connected with the dispatcher.
 * The dispatcher forwards SDUs received from users and PDUs from peers
 * by putting these messages into the appropriate queue, to be processed
 * by the sending or receiving instance.
 *
 * Every instance sends PDUs to the peer by calling send_pdu() and delivers SDUs
 * to the user by calling send_sdu(). Message are read by get_message() from the queue.
 * Timers must be created by create_timer(), armed by set_timer(), disarmed by
 * reset_timer() and deleted by *surprise* delete_timer().
 * Every timer is associated with a message type, which should by distinct from 
 * the other timer message types and the various PDU and SDU types. 
 * When a timer expires, a timer message is put into the queue, 
 * with the type member set to this message type and no additional data.
 *
 *
 * @bug For the message delivery to the user layer @e connected unix domain sockets
 *      are used. Connecting to the user socket may require read/write
 *      permissions to the socket object in the filesystem, 
 *      which can result in the inability of communication 
 *      if the service process is underprivileged.
 *
 *
 * @{
 */

#include "errors.h"
#include "service.h"
#include "sender.h"
#include "receiver.h"

#include <xdt/address.h>

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>


/**
 * @brief Signal handler
 *
 * Terminates the process when SIGINT and SIGTERM is catched.
 */
static void
user_signal_handler(int signo)
{
  exit(0);

  /* suppress 'unused parameter' compiler warning */
  signo = signo;
}


/**
 * @brief Registers a user signal handler
 */
static void
init_signal_handler(void)
{
  struct sigaction sa;
  
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
}

/**
 * @brief Prints program usage information
 *
 * @param f output stream
 * @param cmd command to run the program
 */
static void
print_usage(FILE * f, char const *cmd)
{
  fprintf(f, "usage: %s [-e <error case>] <listen address>\n\n"
             "<error case> = number within %u (no error) and %u\n"
             "<listen address> = host:port\n\n"
             "  host = hostname or IPv4 address in standard dot notation\n"
             "  port = IP port number in range [%d, %d]\n",
          cmd, ERR_NO, ERR_MAX_SUCC - 1, XDT_PORT_MIN, XDT_PORT_MAX);
}

/** 
 * @brief Service program entry funtion
 *
 * The main function translates the given XDT address string
 * into it's binary representation and evaluates 
 * the error case to simulate.
 *
 *
 * Then it calls the message dispatcher.
 * On initial messages the dispatcher sets up 
 * a new sender or receiver instance and returns in a new 
 * process.
 * The appropriate instance entry function
 * start_sender() or start_receiver()
 * is called to process the instance related messages.
 *
 */
int
main(int argc, char *argv[])
{
  XDT_address sap;
  XDT_error error_case = 0;
  int addr_index = 1;

  if (argc < 2) {
    print_usage(stderr, argv[0]);
    return EXIT_FAILURE;
  } else {
    /* check for '-e' */
    if (argv[1][0] == '-' && argv[1][1] == 'e') {
      if (argv[1][2]) {
        /* e.g '-e5' */
        if (!isdigit((int)argv[1][2]) || argv[1][3]) {
          /* e.g. '-ex' or '-e55' */
          error_case = ERR_MAX_SUCC;
        } else {
          error_case = argv[1][2] - '0';
          addr_index = 2;
        }
      } else if (argc > 2 && isdigit((int)argv[2][0])) {
        /* e.g. '-e 5' */
        error_case = argv[2][0] - '0';
        addr_index = 3;
      } else {
        /* e.g. '-e' or '-e x' or '-e 55' */
        error_case = ERR_MAX_SUCC;
      }
    }
  }

  if (error_case >= ERR_MAX_SUCC) {
    fputs("error in <error case>\n", stderr);
    print_usage(stderr, argv[0]);
    return EXIT_FAILURE;
  }
  if (addr_index + 1 != argc) {
    fputs("error in parameter count\n", stderr);
    print_usage(stderr, argv[0]);
    return EXIT_FAILURE;
  }

  if (xdt_address_parse(argv[addr_index], &sap) < 0) {
    fputs("error in <listen address>\n", stderr);
    print_usage(stderr, argv[0]);
    return EXIT_FAILURE;
  }

    
  /* now:
   *
   * sap        - identifies the service instance
   * error_case - contains the error case to simulate
   */

  {
    unsigned conn = 0;
    int role = dispatch(&sap, &conn, error_case);

    init_signal_handler();
  
    switch (role) {

    case XDT_SERVICE_SENDER:
      start_sender();
      break;

    case XDT_SERVICE_RECEIVER:
      start_receiver(conn);
      break;

    default:
      ;
    }
  }

  return EXIT_SUCCESS;
}


/**
 * @}
 */
