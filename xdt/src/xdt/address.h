/**
 * @file address.h
 * @ingroup xdt
 * @brief XDT address type
 */

#ifndef ADDRESS_H
#define ADDRESS_H

/**
 * @addtogroup xdt
 * @{
 */

#include <limits.h>

#include <netinet/in.h>


#ifndef INET_ADDRSTRLEN
/** 
 * @brief Size of a buffer capable to hold an IPv4 address in standard dot notation
 * 
 * Only used on some systems when declaration is missing.
 */
# define INET_ADDRSTRLEN sizeof("111.111.111.111")
#endif

/**
 * @brief Smallest IP port number useable for XDT service implementations
 *
 * @verbatim
 * From /etc/services:
 * # The Well Known Ports are those from 0 through 1023.
 * #
 * # The Registered Ports are those from 1024 through 49151
 * #
 * # The Dynamic and/or Private Ports are those from 49152 through 65535
 * @endverbatim
 */
#define XDT_PORT_MIN 49152

/**
 * @brief Biggest IP port number useable for XDT service implementations
 *
 * See also #XDT_PORT_MIN.
 */
#define XDT_PORT_MAX 65535

/** @brief Smallest XDT slot number used for XDT user implementations */
#define XDT_SLOT_MIN 0

/** @brief Biggest XDT slot number used for XDT user implementations */
#define XDT_SLOT_MAX UINT_MAX


/**
 * @brief XDT address used in SDUs and PDUs
 */
typedef struct
{
  char host[INET_ADDRSTRLEN];   /**< IPv4 address in standard dot notation */
  int port;                     /**< valid IP port number in range [#XDT_PORT_MIN, #XDT_PORT_MAX]*/
  unsigned slot;                /**< XDT user slot in range [#XDT_SLOT_MIN, #XDT_SLOT_MAX] */
} XDT_address;


/**
 * @brief Compares two XDT addresses
 *
 * @param left XDT_address object
 * @param right XDT_address object
 *
 * @return 1 if both addresses are equal, 0 if not
 */
#define XDT_ADDRESS_EQUAL(left, right) !memcmp(&(left), &(right), sizeof(XDT_address))


int xdt_address_to_uap_name(XDT_address const *addr, char *buf, size_t buf_size);
int xdt_address_to_sap_name(XDT_address const *addr, char *buf, size_t buf_size);
int xdt_address_parse(char const *buf, XDT_address * addr);


/**
 * @}
 */

#endif /* ADDRESS_H */
