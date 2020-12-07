/**
 * @file queue.h
 * @ingroup service
 * @brief XDT message queue type
 */

#ifndef QUEUE_H
#define QUEUE_H

/**
 * @addtogroup service
 * @{
 */


#include "pdu.h"

#include <xdt/sdu.h>


/**
 * @brief XDT message queue context
 *
 * Holds message queue information (use as an opaque type).
 */
typedef struct
{
  int id;
} XDT_queue;


int xdt_queue_create(XDT_queue * queue);
int xdt_queue_read(XDT_queue * queue, void *msg, size_t msg_size, int type);
int xdt_queue_write(XDT_queue * queue, void *msg, size_t msg_size);
int xdt_queue_delete(XDT_queue * queue);


/**
 * @}
 */

#endif /* QUEUE_H */
