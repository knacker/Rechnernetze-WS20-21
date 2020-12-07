/**
 * @file user/main.c
 * @ingroup user
 * @brief User program entry point
 */

/** 
 * @defgroup user User layer
 * @brief User Layer implementation
 *
 * All sources in the @e user directory belong to this module.
 *
 * This is the user layer implementation, it can act as a sending user,
 * a @e producer, or as a receiving user, a @e consumer.
 * At startup the command line addresses are parsed with xdt_address_parse().
 * If only one address is given, this one specifies the local address and the process
 * acts as a consumer. If two addresses are given, the first one specifies the local
 * address, the second one the address of the consuming counterpart and the process 
 * acts as a producer.
 *
 * setup_user() registers signal handlers for SIGINT and SIGTERM. If one of these
 * signals is catched, the process can cleanup and quit graceful.
 * A unix domain socket to listen for SDU messages from the XDT layer is created 
 * with the path of the unix address (the user access point, UAP) obtained 
 * by calling xdt_address_to_uap_name() with the local address. 
 * If the process is a producer, a connected unix domain socket for delivering 
 * SDU messages to the XDT layer is created with the path of the unix address 
 * (the service access point, SAP) obtained by calling xdt_address_to_sap_name() 
 * with the local address. The address of the remote consumer is only used in 
 * addressing the first ::XDATrequ SDU.
 *
 * After setup, one of the entry functions start_producer() or start_consumer() is called.
 * From this time, the process behaves like a state machine - receiving SDU messages 
 * and responding properly.
 * To receive SDU messages get_sdu() is called. It blocks until a message from the
 * XDT layer is available. To deliver SDU messages to the XDT layer deliver_sdu()
 * is used. The payload data to send is fetched by successive calls of read_data() 
 * and the received payload data is stord by write_data(). The data is read from
 * standard input (stdin) and written to standard output (stdout). All other output 
 * like debug and error messages are directed to standard error (stderr).
 *
 *
 * @bug For the message delivery to the XDT layer @e connected unix domain sockets
 *      are used. Connecting to the service socket may require read/write permissions 
 *      to the socket object in the filesystem, which can result in the inability 
 *      of communication if the user process is underprivileged.
 *
 * @{
 */

#include <stdio.h>
#include <stdlib.h>

#include "user.h"
#include "producer.h"
#include "consumer.h"


/**
 * @brief Prints program usage information
 *
 * @param f output stream
 * @param cmd command to run the program
 */
static void
print_usage(FILE * f, char const *cmd)
{
  fprintf(f, "usage: %s <local address> [<remote address>]\n\n" "<local address>, <remote address> = host:port[.slot]\n\n" "  host = hostname or IPv4 address in standard dot notation\n" "  port = IP port number in range [%d, %d]\n" "  slot = XDT user slot in range [%u, %u] (default is %u)\n", cmd, XDT_PORT_MIN, XDT_PORT_MAX, XDT_SLOT_MIN, XDT_SLOT_MAX, XDT_SLOT_MIN);
}


/** 
 * @brief User program entry funtion
 *
 * The main function translates the given XDT address string(s)
 * into it's binary representation(s) and sets up a producer or 
 * consumer instance, dependig whether the program is invokend 
 * with one or two addresses.
 * The appropriate instance entry function
 * start_producer() or start_consumer()
 * is called to process the instance related messages.
 */
int
main(int argc, char *argv[])
{
  XDT_address local;
  XDT_address peer;
  int producer = argc > 2;
  int i;

  if (argc < 2 || argc > 3) {
    print_usage(stderr, argv[0]);
    return EXIT_FAILURE;
  }

  if ((i = xdt_address_parse(argv[1], &local)) < 0) {
    fputs("error in <local address>\n", stderr);
    print_usage(stderr, argv[0]);
    return EXIT_FAILURE;
  }

  setup_user(&local, producer);

  if (producer) {
    if (xdt_address_parse(argv[2], &peer) < 0) {
      fputs("error in <remote address>\n", stderr);
      print_usage(stderr, argv[0]);
      return EXIT_FAILURE;
    }

    start_producer(&local, &peer);
  } else {
    start_consumer();
  }


  return EXIT_SUCCESS;
}

/**
 * @}
 */
