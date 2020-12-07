/**
 * @file service.h
 * @ingroup service
 * @brief XDT layer convenient routines
 */

#ifndef SERVICE_H
#define SERVICE_H

/**
 * @addtogroup service
 * @{
 */


#include "pdu.h"
#include "queue.h"
#include "errors.h"

#include <xdt/address.h>
#include <xdt/sdu.h>
#include <xdt/timer.h>


/** @brief Type of the service instance set up by the dispatcher */
typedef enum
{
  XDT_SERVICE_NA, /**< not an instance  */
  XDT_SERVICE_SENDER, /**< sender instance */
  XDT_SERVICE_RECEIVER /**< receiver instance */
} XDT_role;

XDT_role dispatch(XDT_address const *sap, unsigned *c, XDT_error error_case);


/** @brief Message structure to use, when reading from an XDT queue
 * 
 * The message structure offers (by means of the outer union) two different
 * views on the message content:
 * - a generic view, consisting of type header and contained data used to check
 *   message type and to read out timer event IDs from 'type' and
 * - a type specific view which offers (by means of the inner union) access to
 *   the type specific content, which is either a PDU or a SDU.
 * 
 * It is mandatory to check the header 'type' before accessing the type
 * specific fields 'sdu' and 'pdu'.
 */
typedef struct
{
  union
  {
    struct
	{
      long type; /**< type of contained message (SDU, PDU or timer event) */
      char data[(sizeof (XDT_sdu) > sizeof (XDT_pdu)
                ? sizeof (XDT_sdu) : sizeof (XDT_pdu)) - sizeof (long)];
      /**< generic view on message content: byte buffer storing message data (without already contained type header: see field 'type' above) */
    };
    
    union
	{
      XDT_sdu sdu; /**< type specific view on message (incl. type header) if a SDU is contained */
      XDT_pdu pdu; /**< type specific view on message (incl. type header) if a PDU is contained */
    };
  };
} XDT_message;

void send_pdu(XDT_pdu * pdu);
void send_sdu(XDT_sdu * sdu);
void get_message(XDT_message * msg);
void create_timer(XDT_timer * timer, int type);
void set_timer(XDT_timer * timer, double timeout);
void reset_timer(XDT_timer * timer);
void delete_timer(XDT_timer * timer);


/**
 * @}
 */

#endif /* SERVICE_H */
