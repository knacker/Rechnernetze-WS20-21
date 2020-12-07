/**
 * @file sender.c
 * @ingroup service
 * @brief XDT layer sender logic
 */

/**
 * @addtogroup service
 * @{
 */

#include "sender.h"
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
run_sender(void)
{

} /* run_sender */

/** 
 * @brief Sender instance entry function
 *
 * After the dispatcher has set up a new sender instance      
 * and established a message queue between both processes  
 * this function is called to process the messages available    
 * in the message queue.        
 * The only functions and macros needed here are
 * - get_message() to read SDU, PDU and timer messages from the queue
 * - send_sdu() to send an SDU message to the producer,
 * - send_pdu() to send a PDU message to the receiving peer,
 * - #XDT_COPY_DATA to copy the message payload,	 
 * - create_timer() to create a timer associated with a message type,
 * - set_timer() to arm a timer (on expiration a timer associated message is
 *   put into the queue)
 * - reset_timer() to disarm a timer (all timer associated messages are removed
 *   from the queue)         
 * - delete_timer() to delete a timer.      
 */
void
start_sender(void)
{
  run_sender();
} /* start_sender */


/**
 * @}	
 */  