/**
 * @file pdu.c
 * @ingroup service
 * @brief PDU marshalling and debug printing
 */

/**
 * @addtogroup service
 * @{
 */

#include "pdu.h"

#include <ctype.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <unistd.h>

/* see xdr(3) */
#include <rpc/rpc.h>            /* AFAIR needed under SunOS */
#include <rpc/xdr.h>


/**
 * @brief Marshalls an XDT address into/from an XDR encoded byte stream
 *
 * @param xdrs the byte stream associated XDR stream object
 * @param addr points to the XDT address to be marshalled
 * 
 * return 1 on success, 0 on failure
 */
static int
marshal_address(XDR * xdrs, XDT_address * addr)
{
  return xdr_opaque(xdrs, addr->host, sizeof addr->host) && xdr_int(xdrs, &addr->port) && xdr_u_int(xdrs, &addr->slot);
}

/**
 * @brief Marshalls an ABO PDU into/from an XDR encoded byte stream
 *
 * @param xdrs the byte stream associated XDR stream object
 * @param abo points to the ABO PDU to be marshalled
 * 
 * return 1 on success, 0 on failure
 */
static int
marshal_abo(XDR * xdrs, XDT_abo * abo)
{
  /* conn */
  return xdr_u_int(xdrs, &abo->conn);
}

/**
 * @brief Marshalls an ACK PDU into/from an XDR encoded byte stream
 *
 * @param xdrs the byte stream associated XDR stream object
 * @param ack points to the ACK PDU to be marshalled
 * 
 * return 1 on success, 0 on failure
 */
static int
marshal_ack(XDR * xdrs, XDT_ack * ack)
{
  /* sequ
   * [source_addr dest_addr] (if sequ==1)
   * conn
   */

  return xdr_u_int(xdrs, &ack->sequ) && ((ack->sequ == 1) ? (marshal_address(xdrs, &ack->source_addr) && marshal_address(xdrs, &ack->dest_addr)) : 1) && xdr_u_int(xdrs, &ack->conn);
}

/**
 * @brief Marshalls a DT PDU into/from an XDR encoded byte stream
 *
 * @param xdrs the byte stream associated XDR stream object
 * @param dt points to the DT PDU to be marshalled
 * 
 * return 1 on success, 0 on failure
 */
static int
marshal_dt(XDR * xdrs, XDT_dt * dt)
{
  /* sequ
   * source_addr dest_addr (if sequ==1) | conn (if sequ!=1)
   * eom
   * length
   * data
   */

  return xdr_u_int(xdrs, &dt->sequ) && ((dt->sequ == 1) ? (marshal_address(xdrs, &dt->source_addr) && marshal_address(xdrs, &dt->dest_addr)) : xdr_u_int(xdrs, &dt->conn)) && xdr_u_int(xdrs, &dt->eom) && xdr_u_int(xdrs, &dt->length) && xdr_opaque(xdrs, dt->data, dt->length);
}


/**
 * @brief Serializes a PDU message into an XDR encoded byte stream
 *
 * @param pdu points to the PDU message to be serialized
 * @param stream buffer to encode the PDU into
 * @param stream_len number of avaiable bytes in @a stream
 *
 * @return number of written bytes in stream on success, value < 0 on failure
 */
int
serialize_pdu(XDT_pdu * pdu, char *stream, size_t stream_len)
{
  XDR xdrs;
  unsigned pos = 0;

  if (!stream || !stream_len || !pdu) {
    return -2;
  }

  /* create XDT stream object */
  xdrmem_create(&xdrs, stream, stream_len, XDR_ENCODE);

  /* code (as common element in all pdu types)
   * <pdu specific elements>
   */
  switch ((int)pdu->type) {
  case DT:
    if (!xdr_int(&xdrs, &pdu->x.dt.code) || !marshal_dt(&xdrs, &pdu->x.dt)) {
      xdr_destroy(&xdrs);
      return -10;
    }
    break;

  case ACK:
    if (!xdr_int(&xdrs, &pdu->x.ack.code) || !marshal_ack(&xdrs, &pdu->x.ack)) {
      xdr_destroy(&xdrs);
      return -20;
    }
    break;

  case ABO:
    if (!xdr_int(&xdrs, &pdu->x.abo.code) || !marshal_abo(&xdrs, &pdu->x.abo)) {
      xdr_destroy(&xdrs);
      return -30;
    }
    break;

  default:
    return -40;
  }

  /** @bug Use of xdr_getpos() is declared as not portable, but works under the target systems. */
  pos = xdr_getpos(&xdrs);

  xdr_destroy(&xdrs);

  return pos;
}

/**
 * @brief Deserializes a PDU message from an XDR encoded byte stream
 * 
 * @param stream buffer containing the encoded PDU
 * @param stream_len number of bytes in the @a stream
 * @param pdu points to the PDU message to be deserialized
 *
 * @return number of read bytes from stream on success, value < 0 on failure
 */
int
deserialize_pdu(char *stream, size_t stream_len, XDT_pdu * pdu)
{
  XDR xdrs;
  int code;
  unsigned pos = 0;

  if (!stream || !stream_len || !pdu) {
    return -2;
  }

  /* create XDR stream object */
  xdrmem_create(&xdrs, stream, stream_len, XDR_DECODE);

  /* decode code/type */
  if (xdr_int(&xdrs, &code)) {
    pdu->type = code;

    /* decode PDU specific members */
    switch (code) {
    case DT:
      pdu->x.dt.code = code;
      if (!marshal_dt(&xdrs, &pdu->x.dt)) {
        xdr_destroy(&xdrs);
        return -10;
      }
      break;

    case ACK:
      pdu->x.ack.code = code;
      if (!marshal_ack(&xdrs, &pdu->x.ack)) {
        xdr_destroy(&xdrs);
        return -20;
      }
      break;

    case ABO:
      pdu->x.abo.code = code;
      if (!marshal_abo(&xdrs, &pdu->x.abo)) {
        xdr_destroy(&xdrs);
        return -30;
      }
      break;

    default:
      return -40;
    }

    /** @bug Use of xdr_getpos() is declared as not portable, but works under the target systems. */
    pos = xdr_getpos(&xdrs);

    xdr_destroy(&xdrs);

    return pos;
  }

  return -1;
}


/*** DEBUG PRINTING ***************************************************/


/** @brief Wether to print the PDU payload or not */
#define PRINT_PDU_PAYLOAD 0


/**
 * @brief Prints (or not) the PDU payload
 * 
 * When detecting any non-printable characters, the rest of the output
 * is leaved out (indicated by a special tag). Output in general depends 
 * if #PRINT_PDU_PAYLOAD is set.
 *
 * @param data PDU data array
 * @param length used bytes in @a data
 * @param stream output stream, @e stderr is used if @e null
 */
static void
print_pdu_data(char data[XDT_DATA_MAX], unsigned length, FILE * stream)
{
  unsigned i;

  return;

  fputs("data = '", stderr);
  for (i = 0; i < length && i < XDT_DATA_MAX; ++i) {
    if (!isgraph((int)data[i]) && !isspace((int)data[i])) {
      break;
    }
    fputc(data[i], stream);
  }
  if (i < length) {
    fputs("[BINARY DATA FOLLOWS]\n", stderr);
  } else {
    fputs("'\n", stderr);
  }
}

/**
 * @brief Prints the content of a PDU
 *
 * @param pdu points to a @e PDU message
 * @param info string to make the output unique
 * @param stream output stream, @e stderr is used if @e null
 */
void
print_pdu(XDT_pdu * pdu, char *info, FILE * stream)
{
  if (!stream) {
    stream = stderr;
  }

  fprintf(stream, "\nPDU: >> %s << (pid=%d)\n", info ? info : "", (int)getpid());

  switch ((int)pdu->type) {
  case DT:
    fprintf(stream, "type = DT\n");
    if (pdu->x.dt.sequ == 1) {
      fprintf(stream, "source_addr = %s:%d.%u\n", pdu->x.dt.source_addr.host, pdu->x.dt.source_addr.port, pdu->x.dt.source_addr.slot);
      fprintf(stream, "dest_addr = %s:%d.%u\n", pdu->x.dt.dest_addr.host, pdu->x.dt.dest_addr.port, pdu->x.dt.dest_addr.slot);
    } else {
      fprintf(stream, "conn = %u\n", pdu->x.dt.conn);
    }
    fprintf(stream, "sequ = %u\n", pdu->x.dt.sequ);
    fprintf(stream, "eom = %u\n", pdu->x.dt.eom);
    print_pdu_data(pdu->x.dt.data, pdu->x.dt.length, stream);
    fprintf(stream, "length = %u\n", pdu->x.dt.length);
    break;
  case ACK:
    fprintf(stream, "type = ACK\n");
    if (pdu->x.ack.sequ == 1) {
      fprintf(stream, "source_addr = %s:%d.%u\n", pdu->x.ack.source_addr.host, pdu->x.ack.source_addr.port, pdu->x.ack.source_addr.slot);
      fprintf(stream, "dest_addr = %s:%d.%u\n", pdu->x.ack.dest_addr.host, pdu->x.ack.dest_addr.port, pdu->x.ack.dest_addr.slot);
    }
    fprintf(stream, "conn = %u\n", pdu->x.ack.conn);
    fprintf(stream, "sequ = %u\n", pdu->x.ack.sequ);
    break;
  case ABO:
    fprintf(stream, "type = ABO\n");
    fprintf(stream, "conn = %u\n", pdu->x.abo.conn);
    break;
  default:
    fprintf(stream, "type = unknown (%ld)\n", pdu->type);
  }
}


/**
 * @}
 */
