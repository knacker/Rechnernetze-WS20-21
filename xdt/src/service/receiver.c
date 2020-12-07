/**
 * @file receiver.c
 * @ingroup service
 * @brief XDT layer receiver logic
 */

/**
 * @addtogroup service
 * @{
 */

#include "receiver.h"
#include "service.h"
#include <stdlib.h>
#include <stdio.h>


/** @brief connection number of data transfer */
static unsigned conn = 0;


/** 
 * @brief State scheduler
 *
 * Calls the appropriate function associated with the current protocol state.
 */
static void
run_receiver(void)
{
  
} /* run_receiver */


/** 
 * @brief Receiver instance entry function
 *
 * After the dispatcher has set up a new receiver instance
 * and established a message queue between both processes
 * this function is called to process the messages available
 * in the message queue.
 * The only functions and macros needed here are        
 * - get_message() to read SDU, PDU and timer messages from the queue
 * - send_sdu() to send an SDU message to the consumer,     
 * - send_pdu() to send a PDU message to the sending peer,
 * - #XDT_COPY_DATA to copy the message payload,        
 * - create_timer() to create a timer associated with a message type,
 * - set_timer() to arm a timer (on expiration a timer associated message is  
 *   put into the queue)                    
 * - reset_timer() to disarm a timer (all timer associated messages are removed  
 *   from the queue)
 * - delete_timer() to delete a timer.
 *
 * @param connection the connection number assigned to the data transfer
 *        handled by this instance
 */
void
start_receiver(unsigned connection)
{
  conn = connection;
  
  run_receiver();
} /* start_receiver */


/**
 * @}
 */
