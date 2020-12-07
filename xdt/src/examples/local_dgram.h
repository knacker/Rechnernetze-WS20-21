/* local_dgram.h */

#ifndef LOCAL_DGRAM_H
#define LOCAL_DGRAM_H


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/socket.h>
#include <sys/un.h>

#ifndef SUN_LEN
# define SUN_LEN(sun) sizeof(*(sun))
#endif


/* path of unix address the server is receiving datagrams */
#define SERVER_PATH "/tmp/example_server_path"


#endif /* LOCAL_DGRAM_H */
