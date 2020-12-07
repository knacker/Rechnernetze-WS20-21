/**
 * @file user.h
 * @ingroup user
 * @brief User layer convenient routines
 */

#ifndef USER_H
#define USER_H

/**
 * @addtogroup user
 * @{
 */


#include <xdt/address.h>
#include <xdt/sdu.h>


void setup_user(XDT_address * local, int producer);

void get_sdu(XDT_sdu * sdu);
void deliver_sdu(XDT_sdu * sdu);
unsigned read_data(char buffer[XDT_DATA_MAX]);
void write_data(char buffer[XDT_DATA_MAX], unsigned length);


/**
 * @}
 */

#endif /* USER_H */
