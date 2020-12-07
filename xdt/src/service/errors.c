/**
 * @file errors.c
 * @ingroup service
 * @brief send(2) and sendto(2) replacements which simulate error cases
 */

/**
 * @addtogroup service
 * @{
 */

#include "errors.h"
#include "pdu.h"

#include <errno.h>

#include <unistd.h>


/**
 * @brief @e sendto(2) replacement with built-in error case simulation
 *
 * The interface is like the original @e sendto(2) function, but with the @e flags
 * parameter replaced by the @e error @e case to simulate. The behaviour differs in 
 * some ways:
 * - actual transmission depends on the error case specified
 * - if the socket is in a connected state, no ICMP errors are reported
 *
 * For further description you should have a look at the @e sendto(2) manual.
 *
 * @param s socket to use for transmission
 * @param msg message to send
 * @param len length of the message
 * @param error_case error case to simulate
 * @param to address of the target
 * @param tolen size of the address @a to
 * 
 * @return number of characters sent, or -1 if an error occurred
 */
ssize_t
sendto_err(int s, void *msg, size_t len, XDT_error error_case, const struct sockaddr *to, socklen_t tolen)
{
  XDT_pdu pdu;
  ssize_t bytes_sent;

  errno = 0;

  if (deserialize_pdu(msg, len, &pdu) < 0) {
    errno = EINVAL;
    return -1;
  }

  switch (error_case) {
  case ERR_NO:
    /* do nothing */
    break;

  case ERR_DAT1:
    if (pdu.type == DT && pdu.x.dt.sequ == 1) {
      return len;
    }
    break;

  case ERR_DAT2:
    if (pdu.type == DT && pdu.x.dt.sequ == 2) {
      return len;
    }
    break;

  case ERR_DAT4:
    {
      static int first = 1;

      if (first && pdu.type == DT && pdu.x.dt.sequ == 4) {
        first = 0;
        return len;
      }
    }
    break;

  case ERR_DAT3UP:
    if (pdu.type == DT && pdu.x.dt.sequ > 2) {
      return len;
    }
    break;

  case ERR_ACK1:
    if (pdu.type == ACK && pdu.x.ack.sequ == 1) {
      return len;
    }
    break;

  /*
  case ERR_ACK3:
    if (pdu.type == ACK && pdu.x.ack.sequ == 3) {
      return len;
    }
    break;
  */

  case ERR_ACK3:
	{
	  static int first = 1;
	
      if (first && pdu.type == ACK && pdu.x.ack.sequ == 3) {
	    first = 0;
        return len;
      }
	}
    break;

  case ERR_ACK4UP:
    if (pdu.type == ACK && pdu.x.ack.sequ > 3) {
      return len;
    }
    break;

  case ERR_ABO:
    if ((pdu.type == ACK && pdu.x.ack.sequ > 3) || pdu.type == ABO) {
      return len;
    }
    break;

  default:
    /* invalid error case */
    errno = EINVAL;
    return -1;
  }

  if (to && tolen) {
    /* connection-less socket */
    bytes_sent = sendto(s, msg, len, 0, to, tolen);

  } else {
    /* socket in connection mode */
    if ((bytes_sent = write(s, msg, len)) == -1) {
      if (errno == ECONNREFUSED) {
        /* most systems return ICMP errors, but we ignore this */
        return len;
      }
    }
  }

  return bytes_sent;
}

/**
 * @brief @e send(2) replacement with built-in error case simulation
 *
 * The interface is like the original @e send(2) function, but with the @e flags
 * parameter replaced by the @e error @e case to simulate. The behaviour differs in 
 * some ways:
 * - actual transmission depends on the error case specified
 * - no ICMP errors are reported
 *
 * For further description you should have a look at the @e send(2) manual. 
 * (Actually, this function calls sendto_err().)
 *
 * @param s connected socket to use for transmission
 * @param msg message to send
 * @param len length of the message
 * @param error_case error case to simulate
 *
 * @return number of characters sent, or -1 if an error occurred
 */
ssize_t
send_err(int s, void *msg, size_t len, XDT_error error_case)
{
  return sendto_err(s, msg, len, error_case, 0, 0);
}


/**
 * @}
 */
