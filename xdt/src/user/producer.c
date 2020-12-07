/**
 * @file producer.c
 * @ingroup user
 * @brief User layer producer logic
 */

/**
 * @addtogroup user
 * @{
 */

#include "user.h"
#include "producer.h"

#include <assert.h>


enum
{
  IDLE,
  CONNECT,
  BREAK,
  DATA_TRANSFER
};

/** @brief flag indicating if we are already running, when entering the IDLE state */
static int running = 0;

/** @brief current state */
static int state = IDLE;

/** @brief connection number of data transfer */
static unsigned conn = 0;

/** @brief sequence number of last message sent */
static unsigned sequ = 1;

/** @brief flag indicating if we have sent the last message */
static unsigned eom = 0;

/** @brief Local address */
static XDT_address *source_addr = 0;

/** @brief Remote address */
static XDT_address *dest_addr = 0;


/**
 * @brief Implements the producer's IDLE state 
 *
 * When called first time, the #running flag ist set and 
 * the state is immediatly switched to CONNECT.
 * When called second time, the #running flag is cleared.
 */
static void
producer_idle(void)
{
  if (!running) {
    running = 1;
    state = CONNECT;
  } else {
    running = 0;
  }
}


/** @brief Implements the producer's CONNECT state */
static void
producer_connect(void)
{
  XDT_sdu sdu;

  sdu.type = XDATrequ;
  sdu.x.dat_requ.sequ = 1;
  sdu.x.dat_requ.source_addr = *source_addr;
  sdu.x.dat_requ.dest_addr = *dest_addr;
  sdu.x.dat_requ.eom = 0;
  sdu.x.dat_requ.length = read_data(sdu.x.dat_requ.data);
  deliver_sdu(&sdu);

  get_sdu(&sdu);
  if (sdu.type == XDATconf) {
    if (sdu.x.dat_conf.sequ == 1) {
      conn = sdu.x.dat_conf.conn;
      state = DATA_TRANSFER;
    }
  } else if (sdu.type == XABORTind) {
    state = IDLE;
  }
}


/** @brief Implements the producer's BREAK state */
static void
producer_break(void)
{
  XDT_sdu sdu;

  get_sdu(&sdu);

  if (sdu.type == XDATconf) {
    if (sdu.x.dat_conf.conn == conn && sdu.x.dat_conf.sequ == sequ) {
      state = DATA_TRANSFER;
    }
  } else if (sdu.type == XABORTind) {
    if (sdu.x.abort_ind.conn == conn) {
      state = IDLE;
    }
  }
}


/** @brief Implements the producer's DATA_TRANSFER state */
static void
producer_data_transfer(void)
{
  XDT_sdu sdu;

  if (!eom) {
    sdu.type = XDATrequ;
    sdu.x.dat_requ.sequ = ++sequ;
    sdu.x.dat_requ.conn = conn;
    sdu.x.dat_requ.length = read_data(sdu.x.dat_requ.data);
    eom = sdu.x.dat_requ.eom = sdu.x.dat_requ.length < XDT_DATA_MAX;

    deliver_sdu(&sdu);
  }

  for (;;) {
    get_sdu(&sdu);

    switch ((int)sdu.type) {
    case XDATconf:
      if (sdu.x.dat_conf.conn == conn && sdu.x.dat_conf.sequ == sequ) {
        return;
      }
      break;

    case XBREAKind:
      if (sdu.x.break_ind.conn == conn) {
        state = BREAK;
        return;
      }
      break;

    case XDISind:
      if (sdu.x.dis_ind.conn == conn) {
        state = IDLE;
        return;
      }
      break;

    case XABORTind:
      if (sdu.x.abort_ind.conn == conn) {
        state = IDLE;
        return;
      }
      break;
    }
  }
}


/** 
 * @brief State scheduler
 *
 * Calls the appropriate function associated with the current service state.
 */
static void
run_producer(void)
{
  do {
    switch (state) {
    case IDLE:
      producer_idle();
      break;
    case CONNECT:
      producer_connect();
      break;
    case BREAK:
      producer_break();
      break;
    case DATA_TRANSFER:
      producer_data_transfer();
      break;
    }
  } while (running);
}


/** 
 * @brief Producer entry function
 *
 * After set up the environment this function is called to transfer data 
 * and process the messages delivered by the XDT layer.
 *
 * The only functions needed here are
 * - get_sdu() to read SDU messages from the XDT layer,
 * - deliver_sdu() to deliver an SDU message to the XDT layer and
 * - read_data() to read the data to deliver.
 *
 * @param src source address
 * @param dst destination address
 */
void
start_producer(XDT_address * src, XDT_address * dst)
{
  assert(src && dst);

  source_addr = src;
  dest_addr = dst;

  run_producer();
}


/**
 * @}
 */
