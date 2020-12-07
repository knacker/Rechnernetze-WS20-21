/**
 * @file sdu.h
 * @ingroup xdt
 * @brief Common XDT data types
 */

#ifndef SDU_H
#define SDU_H


/**
 * @mainpage
 *
 * This is an implementation of the XDT (eXample Data Transfer) Protocol.
 *
 * It consist of three modules:
 * - @link xdt XDT common types and functions: @endlink
 *   Defines SDU types exchanged between XDT and user layer, formats
 *   how to address the user and XDT instances, access point resolution functions
 *   and a simplified interface to POSIX real-time timers.
 *
 * - @link service XDT layer implementation: @endlink
 *   A more a less complete XDT service.
 *
 * - @link user User layer implementation: @endlink
 *   A complete user acting as producer or consumer.
 *
 *
 * XDT and user layer instances communicate through Unix Domain Sockets,
 * XDT peers by UDP.
 *
 * Every user instance can be identified by it's XDT address, a tripel of
 * (IPv4 address, UDP port, XDT slot), every service instance by the tupel
 * (IPv4 address, UDP port). The interface and port to use for listening for
 * PDUs from peers result from this tupel. The service serves
 * for all user instances whose IP address and port matches the IP address 
 * and port of the service. The XDT slot number exists, to distinguish between 
 * users who use the same service.
 * 
 * The XDT address flows into the path for the unix domain socket address
 * (the access point name), which can be retrieved by resolution functions defined
 * in the @link xdt common @endlink headers.
 *
 */

/**
 * @addtogroup xdt
 * @{
 */

#include "address.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>


/** @brief Maximum size in bytes of SDU payload */
#define XDT_DATA_MAX 255

/**
 * @brief Copy SDU payload
 * 
 * @param src payload array @e data of source SDU
 * @param dst payload array @e data of destination SDU
 * @param len used bytes in source payload array @a src 
 */
#define XDT_COPY_DATA(src, dst, len) assert(len<=XDT_DATA_MAX), memcpy(dst, src, len);


/**
 * @brief SDU message types
 *
 * For use in XDT_sdu.type
 */
enum
{
  sdu_msg_min_pred = 0, /**< lower SDU message area boundary */
  XDATrequ,  /**< type of an XDATrequ SDU message */
  XDATind,   /**< type of an XDATin SDU message */
  XDATconf,  /**< type of an XDATconf SDU message */
  XBREAKind, /**< type of an XBREAKind SDU message */
  XABORTind, /**< type of an XABORTind SDU message */
  XDISind,   /**< type of an XDISind SDU message */
  sdu_msg_max_succ /**< upper SDU message area boundary */
};


/** @brief XDATrequ SDU */
typedef struct
{
  unsigned conn; /**< connection number, ignored if first message (sequence number is 1), else mandatory */
  unsigned sequ; /**< sequence number */
  XDT_address source_addr; /**< source address, mandatory if first message, else ignored */
  XDT_address dest_addr; /**< destination address, mandatory if first message, else ignored */
  unsigned eom; /**< end of message indicator */
  char data[XDT_DATA_MAX]; /**< payload (uninterpreted byte sequence) */
  unsigned length; /**< number of used bytes in payload XDT_xdat_requ.data */
} XDT_xdat_requ;

/** @brief XDATind SDU */
typedef struct
{
  unsigned conn; /**< connection number */
  unsigned sequ; /**< sequence number */
  unsigned eom; /**< end of message indicator */
  char data[XDT_DATA_MAX]; /**< payload (uninterpreted byte sequence) */
  unsigned length; /**< number of used bytes in payload XDT_xdat_ind.data */
} XDT_xdat_ind;

/** @brief XDATconf SDU */
typedef struct
{
  unsigned conn; /**< connection number */
  unsigned sequ; /**< sequence number */
} XDT_xdat_conf;

/** @brief XBREAKind SDU */
typedef struct
{
  unsigned conn; /**< connection number */
} XDT_xbreak_ind;

/** @brief XABORTind SDU */
typedef struct
{
  unsigned conn; /**< connection number, only required i connection was established successfully before */
} XDT_xabort_ind;


/** @brief XDISind SDU */
typedef struct
{
  unsigned conn; /**< connection number */
} XDT_xdis_ind;

/** @brief Union capable of holding any specific SDU */
typedef union
{
  XDT_xdat_requ dat_requ; /**< XDATrequ SDU */
  XDT_xdat_ind dat_ind; /**< XDATind SDU */
  XDT_xdat_conf dat_conf; /**< XDATconf SDU */
  XDT_xbreak_ind break_ind; /**< XBREAKind SDU */
  XDT_xabort_ind abort_ind; /**< XABORTind SDU */
  XDT_xdis_ind dis_ind; /**< XDISind SDU */
} XDT_sdu_x;

/** @brief Compound SDU message */
typedef struct
{
  long type; /**< message type, e.g. ::XDATrequ */
  XDT_sdu_x x; /**< specific SDU */
} XDT_sdu;



void print_sdu(XDT_sdu * sdu, char *info, FILE * stream);


/**
 * @}
 */

#endif /* SDU_H */
