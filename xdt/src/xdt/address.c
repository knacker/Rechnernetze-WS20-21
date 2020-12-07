/**
 * @file address.c
 * @ingroup xdt
 * @brief XDT address conversion
 */

/**
 * @addtogroup xdt
 * @{
 */

#include "address.h"

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <regex.h>

/**
 * @brief Unix Domain Socket path prefix
 *
 * For communication between XDT service and XDT user Unix Domain Sockets
 * are used. The path of the unix address structure always begins with
 * this prefix (so it's easier to find and delete these on errors)
 */
#define XDT_SAP_NAME_PREFIX "/tmp/xdt-"


/**
 * @brief Translates between an XDT address and an User Access Point name
 * 
 * The User Access Point Name is the path of the unix address
 * where the user is listening for messages from the XDT layer.
 *
 * This function can be used by both users and services:
 * A user retrieves his own access point by calling with its local address.
 * A service retrieves the access point of an user instance by
 * calling with the user#s local address.
 *
 * @param addr points to an XDT address representing the User Access Point
 * @param buf memory area where to store the User Access Point name
 * @param buf_size size of the buffer @a buf is pointing to
 *
 * @return 0 on success, value < 0 on failure
 */
int
xdt_address_to_uap_name(XDT_address const *addr, char *buf, size_t buf_size)
{
  int len;

  if (!addr || (addr->port < XDT_PORT_MIN) || addr->port > XDT_PORT_MAX || !buf) {
    return -10;
  }

  /* e.g. "/tmp/xdt-141.43.3.123:58312.5" */
  if ((len = snprintf(buf, buf_size, "%s%s:%d.%u", XDT_SAP_NAME_PREFIX, addr->host, addr->port, addr->slot)) < 0) {
    /* output error */
    return -1;
  }

  if (len >= (int)buf_size) {
    /* buffer to small */
    return -40;
  }

  return 0;
}


/**
 * @brief Translates between an XDT address and a Service Access Point name
 *
 * The Service Access Point Name is the path of the unix address
 * where the service is listening for messages from the user layer.
 *
 * This function can be used by both users and services: 
 * A service retrieves his own access point by calling with its listen address.
 * A user retrieves the access point of its corresponding service instance by
 * calling with its local address.
 *
 * @param addr points to an user or service address
 * @param buf memory area where to store the Service Access Point name
 * @param buf_size size of the buffer @a buf is pointing to
 *
 * @return 0 on success, value < 0 on failure
 */
int
xdt_address_to_sap_name(XDT_address const *addr, char *buf, size_t buf_size)
{
  int len;

  if (!addr || addr->port < XDT_PORT_MIN || addr->port > XDT_PORT_MAX || !buf) {
    return -10;
  }

  /* e.g. "/tmp/xdt-141.43.3.123:58312" */
  if ((len = snprintf(buf, buf_size, "%s%s:%d", XDT_SAP_NAME_PREFIX, addr->host, addr->port)) < 0) {
    /* output error */
    return -1;
  }

  if (len >= (int)buf_size) {
    /* buffer to small */
    return -40;
  }

  return 0;
}

/**
 * @brief Builds an XDT address from it's string representation
 *
 * The string representation of an XDT address in BNF:
 *
 * @verbatim
 *   xdt_address ::= host:port | host:port.slot
 *   host        ::= < hostname or IPv4 address in standard dot notation >
 *   port        ::= < IP port number in range [XDT_PORT_MIN, XDT_PORT_MAX] >
 *   slot        ::= < XDT user slot number in range [XDT_SLOT_MIN, XDT_SLOT_MAX] >
 * @endverbatim
 *
 * @param buf string representing the XDT address
 * @param addr points to an XDT address
 * 
 * @return 0 on success, value < 0 on error
 */
int
xdt_address_parse(char const *buf, XDT_address * addr)
{
  /* host:port[.slot]
   *
   *   host = hostname or IPv4 address in standard dot notation
   *   port = IP port number [XDT_PORT_MIN, XDT_PORT_MAX]
   *   slot = XDT user slot number in range [XDT_SLOT_MIN, XDT_SLOT_MAX] (optional)
   *
   * matching regex: ([^:]{1,})(:)([0-9]{1,})($|\.([0-9]{1,})$)
   */

#define XDT_REGEX "([^:]{1,})(:)([0-9]{1,})($|\\.([0-9]{1,})$)"
#define XDT_MATCH_MAX 6         /* no more are relevant and possible */
#define XDT_MATCH_HOST 1
#define XDT_MATCH_PORT 3
#define XDT_MATCH_SLOT 5

  regex_t preg;
  regmatch_t match[XDT_MATCH_MAX];
  int err = 0;
  char *atom;
  struct hostent *h;
  long l;
  unsigned long ul;

  if (!buf || !addr) {
    return -1;
  }

  if (regcomp(&preg, XDT_REGEX, REG_EXTENDED)) {
    return -10;
  }

  if (regexec(&preg, buf, XDT_MATCH_MAX, match, 0)) {
    return -20;
  }

  if (!(atom = malloc(strlen(buf) + 1))) {
    return -30;
  }

  /* host */
  memcpy(atom, buf + match[XDT_MATCH_HOST].rm_so, match[XDT_MATCH_HOST].rm_eo - match[XDT_MATCH_HOST].rm_so);
  atom[match[XDT_MATCH_HOST].rm_eo - match[XDT_MATCH_HOST].rm_so] = 0;
  if (!(h = gethostbyname(atom)) || h->h_addrtype != AF_INET) {
    err = -40;
    goto FREE_ATOM;
  }
  if (!inet_ntop(AF_INET, h->h_addr_list[0], addr->host, sizeof addr->host)) {
    err = -50;
    goto FREE_ATOM;
  }

  /* port */
  memcpy(atom, buf + match[XDT_MATCH_PORT].rm_so, match[XDT_MATCH_PORT].rm_eo - match[XDT_MATCH_PORT].rm_so);
  atom[match[XDT_MATCH_PORT].rm_eo - match[XDT_MATCH_PORT].rm_so] = 0;
  l = strtol(atom, 0, 10);
  if (l == LONG_MIN || l == LONG_MAX || l < XDT_PORT_MIN || l > XDT_PORT_MAX) {
    err = -60;
    goto FREE_ATOM;
  }
  addr->port = l;

  /* slot (optional, default is XDT_PORT_MIN) */
  if (match[XDT_MATCH_SLOT].rm_so != -1) {
    memcpy(atom, buf + match[XDT_MATCH_SLOT].rm_so, match[XDT_MATCH_SLOT].rm_eo - match[XDT_MATCH_SLOT].rm_so);
    atom[match[XDT_MATCH_SLOT].rm_eo - match[XDT_MATCH_SLOT].rm_so] = 0;
    ul = strtol(atom, 0, 10);
    if (ul == ULONG_MAX) {
      err = -70;
      goto FREE_ATOM;
    }
    /* this next statement looks complicated but surpresses the 
     * 'comparison of unsigned expression < 0 is always false'
     * warning in the special case XDT_SLOT_MIN is zero */
    if (ul > XDT_SLOT_MAX || (XDT_SLOT_MIN > 0 && (ul < 1 || ul + 1 < XDT_SLOT_MIN + 1))) {
      err = -80;
      goto FREE_ATOM;
    }
    addr->slot = ul;
  } else {
    addr->slot = XDT_PORT_MIN;
  }

FREE_ATOM:
  free(atom);
  regfree(&preg);

  return err;
}


/**
 * @}
 */
