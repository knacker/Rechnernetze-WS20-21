/**
 * @file producer.h
 * @ingroup user
 * @brief User layer producer's entry point
 */

#ifndef PRODUCER_H
#define PRODUCER_H

/**
 * @addtogroup user
 * @{
 */


#include <xdt/address.h>


void start_producer(XDT_address * src, XDT_address * dst);


/**
 * @}
 */

#endif /* PRODUCER_H */
