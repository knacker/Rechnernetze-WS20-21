/**
 * @file service.c
 * @ingroup service
 * @brief Service runtime environment
 */

/**
 * @example local_dgram_server.c
 * Simple Unix Domain Socket server.
 */

/**
 * @example local_dgram_client.c
 * Simple Unix Domain Socket server.
 */

/**
 * @example local_dgram.h
 * Common header for Unix Domain Socket example.
 */

/**
 * @example udp_server.c
 * Simple UDP server.
 */

/**
 * @example udp_client.c
 * Simple UDP client.
 */


/**
 * @addtogroup service
 * @{
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


#include "service.h"
#include "queue.h"

#include <xdt/timer.h>

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>


#ifndef SUN_LEN
#define SUN_LEN(sun) sizeof(*(sun))
#endif

/** 
 * @brief Zeroize an object
 *
 * @param buf object to fill with zero bytes
 */
#define ZERO(buf) memset(&(buf), 0, sizeof(buf))

/** 
 * @brief Quit Or Resume
 *
 * Finish dispatching when system call failed except for interruption.
 *
 * @param f function name to pass to @e perror(3)
 */
#define QOR(f) if (errno!=EINTR) { perror(f); should_quit=1; } continue;


/** @brief Number of maximum simultaneous connections to serve */
#define MAX_CONNECTIONS 5

/** @brief Instance context data */
typedef struct
{
  int role; /**< type of instance (sender, receiver or none) */
  pid_t pid; /**< process id of this instance */

  unsigned real_conn; /**< data transfer connection number assigned by the receiver */
  unsigned mapped_conn; /**< mapped local connection number (maybe different from @a real_conn in sender instance) */

  XDT_address producer; /**< source address (only needed for sender instance) */
  XDT_address consumer; /**< destination address (only needed for sender instance) */

  int user_sock; /**< unix domain socket for communication with associated user */
  int peer_sock; /**< UDP socket for communication with associated peer */
  struct sockaddr_in receiver;  /**< sending socket address of receiving peer (only needed for sender instance) */
  socklen_t receiver_len; /**< size of the @a receiver address */

  XDT_queue queue; /**< message queue beween dispatcher and the service instance */
} XDT_instance;

/** @brief Flag indicating the dispatcher should quit */
static volatile sig_atomic_t should_quit = 0;

/** @brief Flag indication that at least one of the spawned instances died */
static volatile sig_atomic_t instance_died = 0;

/** @brief The error case to simulate by all instances */
static XDT_error err_case = 0;

/** @brief UDP socket to receive PDU messages from peers */
static int net_listen_sock = -1;

/** @brief Unix domain socket to receive SDU messages from users */
static int local_listen_sock = -1;

/** @brief Last assigned connection number */
static unsigned int new_conn = 0;

/** @brief Context information for all running instances */
static XDT_instance instances[MAX_CONNECTIONS];

/** @brief Points to the current serving instance */
static XDT_instance *curinst = 0;


/** 
 * @brief Sets up a new receiver instance
 *
 * A random bound UDP socket is created and connected with
 * the sending peer.
 * An unbound unix domain socket is created and connected
 * with the consumer.
 * A message queue is created containing the PDU message @a du.
 * The connection number is assigned.
 *
 * @param du points to the initial DT PDU message
 * 
 * @return 0 on sucess, value < 0 on failure
 */
static int
setup_receiver_instance(XDT_pdu * du)
{
  struct sockaddr_in peer_addr;
  struct sockaddr_un user_addr;
  int i;

  assert(du);

  /* create random bound udp socket and connect with sending peer */
  if ((curinst->peer_sock = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    return -5;
  }
  ZERO(peer_addr);
  peer_addr.sin_family = AF_INET;
  peer_addr.sin_port = htons(du->x.dt.source_addr.port);
  if ((i=inet_pton(AF_INET, du->x.dt.source_addr.host, &peer_addr.sin_addr)) < 1) {
    fprintf(stderr, "inet_pton: %s\n", (i==-1)?strerror(errno):
	    "address does not contain a character string representing a valid IPv4 address");
    return -10;
  }
  if (connect(curinst->peer_sock, (struct sockaddr *)&peer_addr, sizeof peer_addr) == -1) {
    perror("connect");
    return -15;
  }

  /* create unbound local socket and connect with consumer */
  if ((curinst->user_sock = socket(PF_LOCAL, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    return -20;
  }
  ZERO(user_addr);
  user_addr.sun_family = AF_LOCAL;
  if (xdt_address_to_uap_name(&du->x.dt.dest_addr, user_addr.sun_path, sizeof user_addr.sun_path) < 0) {
    fputs("xdt_address_to_uap_name() failed\n", stderr);
    return -25;
  }
  if (connect(curinst->user_sock, (struct sockaddr *)&user_addr, SUN_LEN(&user_addr)) == -1) {
    perror("connect");
    return -30;
  }

  /* create message queue */
  if (xdt_queue_create(&curinst->queue) < 0) {
    return -40;;
  }

  /* put pdu in queue */
  if (xdt_queue_write(&curinst->queue, du, sizeof *du) < 0) {
    return -50;
  }

  /* length of receiving peer socket address (not used in receiver) */
  curinst->receiver_len = 0;

  /* set connection number */
  curinst->real_conn = curinst->mapped_conn = ++new_conn;

  return 0;
}


/** 
 * @brief Sets up a new sender instance
 *
 * The address of the producer is stored.
 * A random bound UDP socket is created and connected with
 * the receiving peer.
 * An unbound unix domain socket is created and connected
 * with the producer.
 * A message queue is created containing the SDU message @a du.
 * The mapped connection number is assigned.
 *
 * @param du points to the initial XDATrequ SDU message
 * 
 * @return 0 on sucess, value < 0 on failure
 */
static int
setup_sender_instance(XDT_sdu * du)
{
  struct sockaddr_in peer_addr;
  struct sockaddr_un user_addr;
  int i;

  assert(du);

  /* store source address */
  curinst->producer = du->x.dat_requ.source_addr;
  curinst->consumer = du->x.dat_requ.dest_addr;


  /* create random bound udp socket and connect with receiving peer */
  if ((curinst->peer_sock = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    return -5;
  }
  peer_addr.sin_family = AF_INET;
  peer_addr.sin_port = htons(du->x.dat_requ.dest_addr.port);
  if ((i=inet_pton(AF_INET, du->x.dat_requ.dest_addr.host, &peer_addr.sin_addr))<1) {
    fprintf(stderr, "inet_pton: %s\n", (i==-1)?strerror(errno):
	    "address does not contain a character string representing a valid IPv4 address");
    return -10;
  }
  if (connect(curinst->peer_sock, (struct sockaddr *)&peer_addr, sizeof peer_addr) == -1) {
    perror("connect");
    return -15;
  }

  /* create unbound local socket and connect with producer */
  if ((curinst->user_sock = socket(PF_LOCAL, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    return -20;
  }
  ZERO(user_addr);
  user_addr.sun_family = AF_LOCAL;
  if (xdt_address_to_uap_name(&du->x.dat_requ.source_addr, user_addr.sun_path, sizeof user_addr.sun_path) < 0) {
    fputs("xdt_address_to_uap_name() failed\n", stderr);
    return -25;
  }
  if (connect(curinst->user_sock, (struct sockaddr *)&user_addr, SUN_LEN(&user_addr)) == -1) {
    perror("connect");
    return -40;
  }

  /* create message queue */
  if (xdt_queue_create(&curinst->queue) < 0) {
    return -50;;
  }

  /* put sdu in queue */
  if (xdt_queue_write(&curinst->queue, du, sizeof *du) < 0) {
    return -60;
  }

  /* length of receiver socket address (to be assigned through 1st ACK) */
  curinst->receiver_len = 0;

  /* set local connection number */
  curinst->mapped_conn = ++new_conn;
  curinst->real_conn = 0;       /* to be assigned by receiver with 1st ACK */

  return 0;
}

/** 
 * @brief Sets up a new instance
 *
 * If not the maximum number of simultaneous connections is reached
 * #curinst will  point to the set up instance.
 *
 * @param role the type of the instance to create
 * @param du initial SDU or PDU message
 *
 * @return 0 on success, value < 0 on failure
 */
static int
setup_instance(XDT_role role, void *du)
{
  int i;

  assert(du);

  curinst = 0;
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    if (instances[i].role == XDT_SERVICE_NA) {
      curinst = &instances[i];
      break;
    }
  }

  if (!curinst) {
    return -10;
  }

  if (role == XDT_SERVICE_RECEIVER) {
    if (setup_receiver_instance(du) < 0) {
      return -20;
    }
  } else {                      /* XDT_SERVICE_SENDER */
    if (setup_sender_instance(du) < 0) {
      return -30;
    }
  }

  /* set instance role */
  curinst->role = role;

  return 0;
}


/**
 * @brief Searches for a receiver instance by it's connection number
 *
 * @param conn connection number
 *
 * @return pointer to the receiver instance with connection number @a conn, else @e null
 */
static XDT_instance *
get_instance_by_real_conn(unsigned conn)
{
  int i;

  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    if (instances[i].role == XDT_SERVICE_RECEIVER && instances[i].real_conn == conn) {
      return &instances[i];
    }
  }

  return 0;
}


/**
 * @brief Searches for a sender instance by it's mapped connection number
 *
 * @param conn mapped connection number
 *
 * @return pointer to the sender instance with mapped connection number @a conn, else @e null
 */
static XDT_instance *
get_instance_by_mapped_conn(unsigned conn)
{
  int i;

  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    if (instances[i].role == XDT_SERVICE_SENDER && instances[i].mapped_conn == conn) {
      return &instances[i];
    }
  }

  return 0;
}


/**
 * @brief Searches for a sender instance by it's endpoint addresses
 *
 * @param src source address
 * @param dst destination address
 *
 * @return pointer to the sender instance with matching addresses, else @e null
 */
static XDT_instance *
get_instance_by_xdt_addresses(XDT_address * src, XDT_address * dst)
{
  int i;

  assert(src && dst);

  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    if (instances[i].role == XDT_SERVICE_SENDER && XDT_ADDRESS_EQUAL(*src, instances[i].producer) && XDT_ADDRESS_EQUAL(*dst, instances[i].consumer)) {
      return &instances[i];
    }
  }

  return 0;
}


/**
 * @brief Searches for a sender instance by it's connection number 
 *        and send address of it's receiving peer
 *
 * @param conn connection number
 * @param addr socket address of the sending peer
 * @param addr_len sizo of the address @a addr
 *
 * @return pointer to the sender instance, else @e null
 */
static XDT_instance *
get_instance_by_socket_address(unsigned conn, struct sockaddr_in *addr, socklen_t addr_len)
{
  int i;

  assert(addr);

  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    if (instances[i].role == XDT_SERVICE_SENDER && instances[i].real_conn == conn && instances[i].receiver_len == addr_len && !memcmp(&instances[i].receiver, addr, addr_len)) {
      return &instances[i];
    }
  }

  return 0;
}


/**
 * @brief Releases context information for a finished instance
 *
 * @param inst points to the instance to cleanup
 */
static void
free_instance(XDT_instance * inst)
{
  if (inst->role != XDT_SERVICE_NA) {
    xdt_queue_delete(&inst->queue);
    inst->role = XDT_SERVICE_NA;
  }
}

/**
 * @brief Releases context information for a finished instance
 *
 * @param pid process id of the instance to cleanup
 */
static void
free_instance_by_pid(pid_t pid)
{
  int i;

  instance_died = 0;
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    if (instances[i].role != XDT_SERVICE_NA && instances[i].pid == pid) {
      free_instance(&instances[i]);
      return;
    }
  }
}

/**
 * @brief Reaps recently deceased instances 
 *
 * Called whenever one or more instances have finsihed.
 * Reads the status of the finsihed processes and frees
 * it's context information.
 */
static void
reap_instances(void)
{
  pid_t pid;

  while ((pid = waitpid(-1, 0, WNOHANG)) > 0) {
    printf("(%d) reaped instance with pid=%d\n", (int)getpid(), (int)pid);
    free_instance_by_pid(pid);
  }
}


/**
 * @brief Detaches current spawned process
 *
 * An new process group is created to detach the instance 
 * from the controlling terminal.
 * The dispatcher has registered handlers for SIGINT, SIGTERM and SIGCHLD.
 * The new spawned instance does not need to catch SIGINT and SIGCHLD.
 * SIGTERM is ignored per default. Since the dispatcher informs all running instance
 * about the end of message dispatching by sending them SIGTERM,
 * the sender and/or receiver code may override this signal action, if necessary.
 */
static void
detach_instance(void)
{
  struct sigaction sa;

  /* create new process group */
  setpgid(0, 0);

  /* reset signal handlers, ignore SIGTERM */
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = SIG_DFL;
  if (sigaction(SIGINT, &sa, 0) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
  if (sigaction(SIGCHLD, &sa, 0) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

  sa.sa_handler = SIG_IGN;
  if (sigaction(SIGTERM, &sa, 0) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
}


/**
 * @brief Signal handler for the dispatcher
 *
 * On SIGINT and SIGTERM, the exit flag #should_quit is set. 
 * On SIGCHLD #instance_died is set.
 *
 * @param signo the raised signal number
 */
static void
dispatcher_signal_handler(int signo)
{
  switch (signo) {
  case SIGINT:
  case SIGTERM:
    should_quit = 1;
    break;

  case SIGCHLD:
    instance_died = 1;
    break;
  }
}


/** @brief Sets up signal handlers for the dispatcher */
static void
setup_signals(void)
{
  struct sigaction sa;

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_NOCLDSTOP;
#ifdef SA_INTERRUPT
  /* do not restart system calls after those signals */
  sa.sa_flags |= SA_INTERRUPT;
#endif
  sa.sa_handler = dispatcher_signal_handler;

  if (sigaction(SIGINT, &sa, 0) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
  if (sigaction(SIGTERM, &sa, 0) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
  if (sigaction(SIGCHLD, &sa, 0) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
}


/**
 * @brief Debug printing of timer messages
 *
 * Analogous to print_sdu() and print_pdu().
 *
 * @param msg points to the timer message
 * @param info string to make the output unique
 * @param stream output stream, @e stderr is used if @e null
 */
static void
print_timer(XDT_message * msg, char *info, FILE * stream)
{
  if (!stream) {
    stream = stderr;
  }

  fprintf(stream, "\nTIMER: >> %s <<\n", info ? info : "");
  fprintf(stream, "type = (%ld)\n", msg->type);
}


/**
 * @brief Signal handler for real-time signals
 *
 * Called on timer expiration.
 * A message with the timers type is put into the queue.
 *
 * @param signo raised signal number 
 * @param info signal information
 * @param cruft not used
 */
static void
timeout_handler(int signo, siginfo_t * info, void *cruft)
{
  int saved_errno = errno;
  long type = info->si_value.sival_int;

  /* avoid 'unused parameter' compiler warnings */
  signo = signo;
  cruft = 0;

  /** 
   * @bug The @e msgsnd(2) system call used in xdt_queue_write() 
   *      is not declared as reentrant (async-signal-safe), 
   *      so to behaviour is undefined.
   */
  if (xdt_queue_write(&curinst->queue, &type, sizeof type) < 0) {
    fputs("writing queue failed\n", stderr);
    exit(EXIT_FAILURE);
  }

  errno = saved_errno;
}


/*** PUBLIC *************************************************************/


/**
 * @brief Message dispatcher
 *
 * The message dispatcher creates listening sockets for receiving messages from
 * peers and users. Whenever a message is received the dispatcher spawns a 
 * new instance inter-connected with a message queue and/or puts the message
 * into the appropriate message queue to be processed by the instance.
 * If the message is the 1st ACK of a connection, the connection number is stored
 * and whenever a message from a producer arrives, the mapped connection
 * number is replaced by the real connection number before putting the message
 * into the queue.
 * Some signals are catched to graceful terminate the dispatcher and all still running 
 * instances (by sending them SIGTERM, which are ignored there per default).
 * The initial connection number is random generated and each instance gets assigned
 * a consecutive connection number (mapped or real).
 *
 * @param sap local listen address
 * @param c when returning as receiver, the assigned connection number 
 *        is stored where @a c points to
 * @param error_case the error case to simulate by all instances
 *
 * @return 
 */
XDT_role
dispatch(XDT_address const *sap, unsigned *c, XDT_error error_case)
{
  struct sockaddr_in net_addr, peer_addr;
  struct sockaddr_un local_addr, user_addr;
  socklen_t addr_len;
  fd_set master_set;
  XDT_pdu pdu;
  XDT_sdu sdu;
  char pdu_stream[PDU_STREAM_MAX];
  int i;

  printf("(%d) dispatching messages started...\n", (int)getpid());

  if (!sap || !c) {
    fputs("parameter failure\n", stderr);
    exit(EXIT_FAILURE);
  }

  err_case = error_case;

  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    instances[i].role = XDT_SERVICE_NA;
  }

  setup_signals();

  /* init starting connection number */
  srand(time(0) ^ getpid());
  new_conn = rand();

  /* create peer endpoint */
  if ((net_listen_sock = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }
  ZERO(net_addr);
  net_addr.sin_family = AF_INET;
  net_addr.sin_port = htons(sap->port);
  if ((i=inet_pton(AF_INET, sap->host, &net_addr.sin_addr)) <1) {
    fprintf(stderr, "inet_pton: %s\n", (i==-1)?strerror(errno):
	    "address does not contain a character string representing a valid IPv4 address");
    exit(EXIT_FAILURE);
  }
  if (bind(net_listen_sock, (struct sockaddr *)&net_addr, sizeof net_addr) == -1) {
    perror("bind");
    fputs("Maybe another service is running using the same SAP\n", stderr);
    exit(EXIT_FAILURE);
  }

  /* create user endpoint */
  if ((local_listen_sock = socket(PF_LOCAL, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }
  ZERO(local_addr);
  local_addr.sun_family = AF_LOCAL;
  if (xdt_address_to_sap_name(sap, local_addr.sun_path, sizeof local_addr.sun_path) < 0) {
    fputs("xdt_address_to_sap_name() failed\n", stderr);
    exit(EXIT_FAILURE);
  }
  /* fails, if path from previous run not deleted */
  if (bind(local_listen_sock, (struct sockaddr *)&local_addr, SUN_LEN(&local_addr)) == -1) {
    perror("bind");
    fprintf(stderr, "Possible reasons:\n- another service is running using the same SAP\n- a previous run exited unclean, try to remove '%s'\n", local_addr.sun_path);
    exit(EXIT_FAILURE);
  }

  i = 1 + ((net_listen_sock > local_listen_sock) ? net_listen_sock : local_listen_sock);
  FD_ZERO(&master_set);
  FD_SET(net_listen_sock, &master_set);
  FD_SET(local_listen_sock, &master_set);

  while (!should_quit) {
    fd_set sock_set = master_set;

    /* reap recently deceased instances */
    reap_instances();

    /* wait for readable socket */
    if (select(i, &sock_set, 0, 0, 0) == -1) {
      QOR("select");
    }

    if (FD_ISSET(net_listen_sock, &sock_set)) {

      /* pdu from peer */
      addr_len = sizeof peer_addr;
      if (recvfrom(net_listen_sock, pdu_stream, sizeof pdu_stream, 0, (struct sockaddr *)&peer_addr, &addr_len) == -1) {
        QOR("recvfrom");
      }
      if (deserialize_pdu(pdu_stream, sizeof pdu_stream, &pdu) < 0) {
        fputs("deserializing PDU failed\n", stderr);
        break;
      }

      switch ((int)pdu.type) {
      case DT:
        /* I'm receiver */
        if (pdu.x.dt.sequ == 1) {
          /* initial DT */
          if (setup_instance(XDT_SERVICE_RECEIVER, &pdu) == 0) {
            *c = curinst->real_conn;
            switch (curinst->pid = fork()) {
            case 0:
              detach_instance();
              return XDT_SERVICE_RECEIVER;
            case -1:
              QOR("fork");
              break;
            default:
              /* parent */
              printf("(%d) forked receiver instance with pid=%d\n", (int)getpid(), (int)curinst->pid);
            }
          } else {
            fputs("warning: could not setup receiver instance\n", stderr);
          }
        } else {
          /* not initial DT */
          if (!(curinst = get_instance_by_real_conn(pdu.x.dt.conn))) {
            fputs("warning: get_instance_by_real_conn: could not find instance for received DT\n", stderr);
            continue;
          }
          if (xdt_queue_write(&curinst->queue, &pdu, sizeof pdu) < 0) {
            QOR("xdt_queue_write");
          }
        }
        break;

      case ACK:
        /* I'm sender */
        if (pdu.x.ack.sequ == 1) {
          /* initial ACK */
          if (!(curinst = get_instance_by_xdt_addresses(&pdu.x.ack.dest_addr, &pdu.x.ack.source_addr))) {
            fputs("warning: get_instance_by_xdt_addresses: could not find instance for received ACK\n", stderr);
            continue;
          }
          /* store connection number */
          curinst->real_conn = pdu.x.ack.conn;

          /* store socket address of receiving peer */
          memcpy(&curinst->receiver, &peer_addr, addr_len);
          curinst->receiver_len = addr_len;

          /* deliver message */
          if (xdt_queue_write(&curinst->queue, &pdu, sizeof pdu) < 0) {
            QOR("xdt_queue_write");
          }
        } else {
          /*not initial ACK */
          if (!(curinst = get_instance_by_socket_address(pdu.x.ack.conn, &peer_addr, addr_len))) {
            fputs("warning: get_instance_by_socket_address: could not find instance for received ACK\n", stderr);
            break;
          }
          if (xdt_queue_write(&curinst->queue, &pdu, sizeof pdu) < 0) {
            QOR("xdt_queue_write");
          }
        }
        break;

      case ABO:
        /* I'm sender */
        if (!(curinst = get_instance_by_socket_address(pdu.x.abo.conn, &peer_addr, addr_len))) {
          fputs("warning: get_instance_by_socket_address: could not find instance for received ABO\n", stderr);
          break;
        }
        if (xdt_queue_write(&curinst->queue, &pdu, sizeof pdu) < 0) {
          QOR("xdt_queue_write");
        }
        break;

      default:
        fputs("warning: unknown PDU type\n", stderr);
      }
    }

    if (FD_ISSET(local_listen_sock, &sock_set)) {
      /* sdu from user */
      addr_len = sizeof user_addr;
      if (recvfrom(local_listen_sock, &sdu, sizeof sdu, 0, (struct sockaddr *)&user_addr, &addr_len) == -1) {
        QOR("recvfrom");
      }

      if (sdu.type == XDATrequ) {
        if (sdu.x.dat_requ.sequ == 1) {
          /* initial XDATrequ */
          if (setup_instance(XDT_SERVICE_SENDER, &sdu) == 0) {
            switch (curinst->pid = fork()) {
            case 0:
              detach_instance();
              return XDT_SERVICE_SENDER;
            case -1:
              QOR("fork");
              break;
            default:
              /* parent */
              printf("(%d) forked sender instance with pid=%d\n", (int)getpid(), (int)curinst->pid);
            }
          } else {
            fputs("warning: could not setup sender instance\n", stderr);
          }
        } else {
          /* not initial XDATrequ */

          /* get instance data */
          if (!(curinst = get_instance_by_mapped_conn(sdu.x.dat_requ.conn))) {
            fputs("warning: get_instance_by_mapped_conn: could not find instance for received XDATrequ\n", stderr);
            continue;
          }

          /* set connection number to real connection number */
          sdu.x.dat_requ.conn = curinst->real_conn;

          /* deliver message */
          if (xdt_queue_write(&curinst->queue, &sdu, sizeof sdu) < 0) {
            QOR("xdt_queue_write");
          }
        }
      } else {
        fputs("warning: unknown SDU type\n", stderr);
      }
    }
  }

  /* cleanup */
  printf("(%d) ...dispatching messages finished. Inform running instances...\n", (int)getpid());

  /* terminate still running instances */
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    if (instances[i].role != XDT_SERVICE_NA) {
      printf("(%d) send SIGTERM to instance with pid=%d\n", (int)getpid(), (int)instances[i].pid);
      kill(instances[i].pid, SIGTERM);
    }
  }

WAIT:
  for (i = 0; i < MAX_CONNECTIONS; ++i) {
    if (instances[i].role != XDT_SERVICE_NA) {
      pid_t pid;

      if ((pid = waitpid(-1, 0, 0)) == -1) {
        if (errno != EINTR) {
          perror("waitpid");
          exit(EXIT_FAILURE);
        }
        goto WAIT;
      }
      printf("(%d) reaped instance with pid=%d\n", (int)getpid(), (int)pid);
      free_instance_by_pid(pid);
      goto WAIT;
    }
  }

  remove(local_addr.sun_path);

  printf("(%d) ...done.\n", (int)getpid());

  return XDT_SERVICE_NA;
}


/**
 * @brief Sends a PDU to the peer
 *
 * @param pdu points to the PDU message
 */
void
send_pdu(XDT_pdu * pdu)
{
  char pdu_stream[PDU_STREAM_MAX];

  print_pdu(pdu, "to send", 0);

  if (serialize_pdu(pdu, pdu_stream, sizeof pdu_stream) < 0) {
    fputs("serializing PDU failed\n", stderr);
    exit(EXIT_FAILURE);
  }
  if (send_err(curinst->peer_sock, pdu_stream, sizeof pdu_stream, err_case) == -1) {
    perror("send_err");
  }
}

/**
 * @brief Sends an SDU to the user
 *
 * When called by a sender instance, the connection number is mapped before transmission.
 *
 * @param sdu points to the SDU message 
 */
void
send_sdu(XDT_sdu * sdu)
{
  if (curinst->role == XDT_SERVICE_SENDER) {

    print_sdu(sdu, "to send /before/ connection mapping", 0);

    /* map connection number */
    switch ((int)sdu->type) {
    case XDATconf:
      sdu->x.dat_conf.conn = curinst->mapped_conn;
      break;
    case XBREAKind:
      sdu->x.break_ind.conn = curinst->mapped_conn;
      break;
    case XABORTind:
      sdu->x.abort_ind.conn = curinst->mapped_conn;
      break;
    case XDISind:
      sdu->x.dis_ind.conn = curinst->mapped_conn;
      break;
    }

    print_sdu(sdu, "to send /after/ connection mapping", 0);

  } else {
    print_sdu(sdu, "to send", 0);
  }

  if (write(curinst->user_sock, sdu, sizeof *sdu) == -1) {
    perror("warning: send_sdu: write");
  }
}

/**
 * @brief Get the next PDU, SDU or timer message 
 *
 * When the call is interrupted by a signal, the message type is set to 0.
 *
 * @param msg points to the message buffer 
 *
 * @note The function does not necessarily return, when the process gets
 *       a signal. Thus you should not rely on this. Instead you should
 *       simply ignore messages of type 0 and repeat the call of function
 *       get_message() in this case.
 */
void
get_message(XDT_message * msg)
{
  if (xdt_queue_read(&curinst->queue, msg, sizeof *msg, 0) < 0) {
    if (errno != EINTR) {
      perror("get_message: reading queue failed");
      exit(EXIT_FAILURE);
    }
    /* interrupted, clear type */
    msg->type = 0;
  }

  if (msg->type > sdu_msg_min_pred && msg->type < sdu_msg_max_succ) {
    print_sdu(&(msg->sdu), "received", 0);

  } else if (msg->type > pdu_msg_min_pred && msg->type < pdu_msg_max_succ) {
    print_pdu(&(msg->pdu), "received", 0);

  } else if (msg->type > pdu_msg_max_succ) {
    print_timer(msg, "expired", 0);
  }
}




/**
 * @brief Creates an instance specific timer
 *
 * When the timer is armed and expires, a timer message is delivered by the get_message() 
 * function, with no data but the message type set to the @a type value.
 *
 * @param timer points to an XDT timer object
 * @param type type value associated with this timer (should be distinct from SDU and PDU types)
 */
void
create_timer(XDT_timer * timer, int type)
{
  if (type <= pdu_msg_max_succ) {
    perror("creating timer failed (invalid type value)");
    exit(EXIT_FAILURE);
  }
  if (xdt_timer_create(timer, TIMER_SIGNAL_BASE, timeout_handler, type) < 0) {
    perror("creating timer failed");
    exit(EXIT_FAILURE);
  }
}


/**
 * @brief Arms an instance specific timer
 * 
 * @param timer points to an XDT timer
 * @param timeout number of seconds after the timer should expire
 */
void
set_timer(XDT_timer * timer, double timeout)
{
  if (xdt_timer_set(timer, timeout) < 0) {
    fputs("setting timer failed\n", stderr);
    exit(EXIT_FAILURE);
  }
}

/**
 * @brief Disarms an instance specific timer
 * 
 * Additionally, all timer messages associated with this timer still available 
 * in the message queue are removed.
 * 
 * @param timer points to an XDT timer
 */
void
reset_timer(XDT_timer * timer)
{
  long type;
  int i;

  if (xdt_timer_reset(timer) < 0) {
    fputs("resetting timer failed\n", stderr);
    exit(EXIT_FAILURE);
  }

  /* remove all timer messages of given type, continue on interrupt */
  while ((i = xdt_queue_read(&curinst->queue, &type, sizeof type, timer->type)) > 0 || (i < 0 && errno == EINTR));

  if (i < 0) {
    perror("reset_timer: reading queue failed");
    exit(EXIT_FAILURE);
  }
}

/**
 * @brief Deletes an instance specific timer
 *
 * Additionally, all timer messages associated with this timer still available 
 * in the message queue are removed.
 *
 * @param timer points to an XDT timer
 */
void
delete_timer(XDT_timer * timer)
{
  reset_timer(timer);

  if (xdt_timer_delete(timer) < 0) {
    fputs("deleting timer failed\n", stderr);
    exit(EXIT_FAILURE);
  }
}


/**
 * @}
 */
