/**
 * @file queue.c
 * @ingroup service
 * @brief XDT message queue implementation
 *
 * This file implements a System V IPC message queue wrapper. Functions of interest may be
 * @e msgget(2), @e msgrcv(2), @e msgsnd(2) and @e msgctl(2).
 *
 * All messages put into the queue @b must contain at first a
 * @e long value greater than 0, followed by a byte array with the actual data.
 *
 * Example:
 * @code
 * struct example_message{
 *   long type;
 *   char data[4711];
 * };
 * @endcode
 */

/**
 * @addtogroup service
 * @{
 */


#include "queue.h"

#include <errno.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>


/**
 * @brief Creates an XDT queue
 *
 * @param queue points to an XDT queue object
 *
 * @return 0 on success, value < 0 on error (-1 if @e msgget(2) failed)
 */
int
xdt_queue_create(XDT_queue * queue)
{
  errno = 0;

  if (!queue) {
    errno = EINVAL;
    return -2;
  }

  return (queue->id = msgget(IPC_PRIVATE, 0600)) == -1 ? -1 : 0;
}


/**
 * @brief Reads a message from an XDT queue
 * 
 * @param queue points to an XDT queue
 * @param msg points to the buffer where to store the message
 * @param msg_size size of the buffer @a msg points to
 * @param type message type you are interested in (0 if no special interests)
 *
 * If type is 0, the function blocks until the next message is available.
 * If type is greater than 0, the function immediatly returns, regardless of if such a message is available.
 *
 * @return 
 * - on success size of message (type and data), so the value is at least the size of a @e long
 * - 0 if type was set and no such message is available
 * - value < 0 on error (-1 if @e msgrcv(2) failed)
 */
int
xdt_queue_read(XDT_queue * queue, void *msg, size_t msg_size, int type)
{
  ssize_t bytes_read;

  errno = 0;

  if (msg_size < sizeof (long)) {
    errno = EINVAL;
    return -2;
  }

  if ((bytes_read = msgrcv(queue->id, msg, msg_size - sizeof (long), type, type ? IPC_NOWAIT : 0)) == -1) {

    if (type && errno == ENOMSG) {
      return 0;
    }

    return -1;
  }

  return bytes_read + sizeof (long);
}


/**
 * @brief Writes a message to an XDT queue
 * 
 * @param queue points to an XDT queue
 * @param msg points to the message
 * @param msg_size size of the message (type and data) @a msg,
 *        so the value must be at least the size of a @e long
 *
 * @return 0 on success, value < 0 on error (-1 if @e msgsnd(2) failed)
 */
int
xdt_queue_write(XDT_queue * queue, void *msg, size_t msg_size)
{
  errno = 0;

  if (!queue || !msg || msg_size < sizeof (long)) {
    return -2;
  }
  return msgsnd(queue->id, msg, msg_size - sizeof (long), 0);
}

/**
 * @brief Deletes the XDT queue
 *
 * @return 0 on success, value < 0 on error (-1 if @e msgctl(2) failed)
 */
int
xdt_queue_delete(XDT_queue * queue)
{
  int id;

  errno = 0;

  if (!queue) {
    return -2;
  }

  id = queue->id;
  queue->id = -1;
  return msgctl(id, IPC_RMID, 0);
}


/**
 * @}
 */
