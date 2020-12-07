/**
 * @file pdu.h
 * @ingroup service
 * @brief Protocol Data Types
 */

#ifndef PDU_H
#define PDU_H

/**
 * @addtogroup service
 * @{
 */

#include <xdt/sdu.h>
#include <xdt/address.h>

/**
 * @brief PDU message types
 *
 * For use in XDT_pdu.type
 */
enum
{
  pdu_msg_min_pred = sdu_msg_max_succ, /**< lower PDU message area boundary */
  DT, /**< type of an DT SDU message */
  ACK, /**< type of an ACK SDU message */
  ABO, /**< type of an ABO SDU message */
  pdu_msg_max_succ /**< upper PDU message area boundary */
};

/** @brief DT PDU */
typedef struct
{
  int code; /**< PDU type, must be ::DT */
  XDT_address source_addr;  /**< source address, mandatory if first message, else ignored */
  XDT_address dest_addr; /**< destination address, mandatory if first message, else ignored */
  unsigned conn;  /**< connection number, ignored if first message (sequence number is 1), else mandatory */
  unsigned sequ; /**< sequence number */
  unsigned eom; /**< end of message indicator */
  char data[XDT_DATA_MAX]; /**< payload (uninterpreted byte sequence) */
  unsigned length; /**< number of used bytes in payload XDT_dt.data */
} XDT_dt;

/** @brief ACK PDU */
typedef struct
{
  int code; /**< PDU type, must be ::ACK */
  XDT_address source_addr; /**< source address, mandatory if first message, else ignored */
  XDT_address dest_addr; /**< destination address, mandatory if first message, else ignored */
  unsigned conn; /**< connection number, to be set to the given conn value by the receiver instance if first message!!! */
  unsigned sequ; /**< sequence number */
} XDT_ack;

/** @brief ABO PDU */
typedef struct
{
  int code; /**< PDU type, must be ::ABO */
  unsigned conn; /**< connection number */
} XDT_abo;

/** @brief Union capable of holding any specific PDU */
typedef union
{
  XDT_dt dt; /**< DT PDU */
  XDT_ack ack; /**< ACK PDU */
  XDT_abo abo; /**< ABO PDU */
} XDT_pdu_x;

/** @brief Compound PDU message */
typedef struct
{
  long type; /**< message type, e.g. ::DT */
  XDT_pdu_x x; /**< specific PDU */
} XDT_pdu;


/**
 * @brief Maximal size of an XDR encoded PDU
 *
 * XDR encodes each item in max 4 bytes, so we take an approximation on the biggest message.
 */
#define PDU_STREAM_MAX (4 * sizeof(XDT_pdu_x))


int serialize_pdu(XDT_pdu * pdu, char *stream, size_t stream_len);
int deserialize_pdu(char *stream, size_t stream_len, XDT_pdu * pdu);
void print_pdu(XDT_pdu * pdu, char *info, FILE * stream);


/**
 * @}
 */

#endif /* PDU_H */
