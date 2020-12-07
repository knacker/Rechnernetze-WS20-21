/**
 * @file sdu.c
 * @ingroup xdt
 * @brief SDU debug printing
 */

/** 
 * @defgroup xdt XDT Common
 *
 * @brief The functions and types of this module are used by both XDT layer and user layer implementations
 *
 * All sources in the @e xdt directory belong to this module.
 *
 * In timer.h, timer.c you can find a POSIX real-time timer wrapper. 
 * All you need is a timer variable of type ::XDT_timer, 
 * a signal_handler of type ::timeout_handler_func which is called on timer expiration,
 * a signal number which should be raised on expiration and 
 * a unique message type that belongs to this timer. 
 * When given this values to xdt_timer_create() you can arm and disarm this timer 
 * with xdt_timer_set() and xdt_timer_reset().
 * If you do not need the timer anymore, remove the timer with xdt_timer_delete().
 * Look in in the example section how to use the timer functions.
 *
 *
 * In sdu.h, sdu.c the common SDU types used in user layer and XDT layer implementations 
 * are defined. Both layers exchange ::XDT_sdu SDU messages, 
 * which consist of a SDU type member, e.g. ::XDATrequ, and a union ::XDT_sdu_x 
 * containing the specific SDU, e.g. XDT_xdat_requ.
 * Also, a function print_sdu() to uniformly print SDU messages is available.
 *
 * 
 * In address.h, address.c the address type used in SDUs are defined.
 * An XDT_address consist of an IPv4 host address in standard dot notation,
 * an IP port number and an XDT user slot.
 * The tupel (host, port) identifies an XDT layer process,
 * the tripel (host, port, slot) identifies an user layer process.
 * To convert an XDT address string into it's internal representation
 * xdt_address_parse() is available. This address is needed to get the 
 * access points of the several processes - xdt_address_to_uap_name() to get the
 * name of the user access point of an user layer process, xdt_address_to_sap_name() to
 * get the name of the service access point of an XDT layer process. These names are used
 * as the path in the unix address to create and access listening Unix Domain Sockets.
 *
 * @{
 */


#include "sdu.h"

#include <ctype.h>

#include <sys/types.h>
#include <unistd.h>


/** @brief Wether to print the SDU payload or not */
#define PRINT_SDU_PAYLOAD 0


/**
 * @brief Prints (or not) the SDU payload
 * 
 * When detecting any non-printable characters, the rest of the output
 * is leaved out (indicated by a special tag). Output in general depends 
 * if #PRINT_SDU_PAYLOAD is set.
 *
 * @param data SDU data array
 * @param length used bytes in @a data
 * @param stream output stream, @e stderr is used if @e null
 */
static void
print_sdu_payload(char data[XDT_DATA_MAX], unsigned length, FILE * stream)
{
  unsigned i;

  if (PRINT_SDU_PAYLOAD) {

    fputs("data = '", stderr);
    for (i = 0; (i < length) && (i < XDT_DATA_MAX); ++i) {
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
}


/**
 * @brief Prints the content of an SDU
 *
 * @param sdu points to an @e SDU message
 * @param info string to make the output unique
 * @param stream output stream, @e stderr is used if @e null
 */
void
print_sdu(XDT_sdu * sdu, char *info, FILE * stream)
{
  if (!stream) {
    stream = stderr;
  }

  fprintf(stream, "\nSDU: >> %s << (pid=%d)\n", info ? info : "", (int)getpid());

  switch ((int)sdu->type) {
  case XDATrequ:
    fprintf(stream, "type = XDATrequ\n");
    if (sdu->x.dat_requ.sequ == 1) {
      fprintf(stream, "source_addr = %s:%d.%u\n", sdu->x.dat_requ.source_addr.host, sdu->x.dat_requ.source_addr.port, sdu->x.dat_requ.source_addr.slot);
      fprintf(stream, "dest_addr = %s:%d.%u\n", sdu->x.dat_requ.dest_addr.host, sdu->x.dat_requ.dest_addr.port, sdu->x.dat_requ.dest_addr.slot);
    } else {
      fprintf(stream, "conn = %u\n", sdu->x.dat_requ.conn);
    }
    fprintf(stream, "sequ = %u\n", sdu->x.dat_requ.sequ);
    fprintf(stream, "eom = %u\n", sdu->x.dat_requ.eom);
    print_sdu_payload(sdu->x.dat_requ.data, sdu->x.dat_requ.length, stream);
    fprintf(stream, "length = %u\n", sdu->x.dat_requ.length);
    break;
  case XDATind:
    fprintf(stream, "type = XDATind\n");
    fprintf(stream, "conn = %u\n", sdu->x.dat_ind.conn);
    fprintf(stream, "sequ = %u\n", sdu->x.dat_ind.sequ);
    fprintf(stream, "eom = %u\n", sdu->x.dat_ind.eom);
    print_sdu_payload(sdu->x.dat_ind.data, sdu->x.dat_ind.length, stream);
    fprintf(stream, "length = %u\n", sdu->x.dat_ind.length);
    break;
  case XDATconf:
    fprintf(stream, "type = XDATconf\n");
    fprintf(stream, "conn = %u\n", sdu->x.dat_conf.conn);
    fprintf(stream, "sequ = %u\n", sdu->x.dat_conf.sequ);
    break;
  case XBREAKind:
    fprintf(stream, "type = XBREAKind\n");
    fprintf(stream, "conn = %u\n", sdu->x.break_ind.conn);
    break;
  case XABORTind:
    fprintf(stream, "type = XABORTind\n");
    fprintf(stream, "conn = %u\n", sdu->x.abort_ind.conn);
    break;
  case XDISind:
    fprintf(stream, "type = XDISind\n");
    fprintf(stream, "conn = %u\n", sdu->x.dis_ind.conn);
    break;
  default:
    if (sdu->type == 0) {
      fputs("<interrupted by timer arrival>\n", stream);
    } else {
      fprintf(stream, "type = %ld (maybe timer)\n", sdu->type);
    }
  }
}


/**
 * @}
 */
