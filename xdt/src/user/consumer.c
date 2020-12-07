/**
 * @file consumer.c
 * @ingroup user
 * @brief User layer consumer logic
 */

/**
 * @addtogroup user
 * @{
 */

#include "consumer.h"
#include "user.h"

#include <assert.h>

/** @brief Service states */
enum
{
  IDLE,
  CONNECT,
  DATA_TRANSFER
};


/** @brief Flag indicating if we are already running, when entering the IDLE state */
static int running = 0;

/** @brief Current state */
static int state = IDLE;

/** @brief Connection number of data transfer */
static unsigned conn = 0;

/** @brief Sequence number of next message to send */
static unsigned sequ = 1;


/**
 * @brief Implements the consumer's IDLE state 
 *
 * When called first time, the #running flag is set and 
 * the state is immediatly switched to CONNECT.
 * When called second time, the #running flag is cleared.
 */
static void
consumer_idle(void)
{
  if (!running) {
    running = 1;
    state = CONNECT;
  } else {
    running = 0;
  }
}


/** @brief Implements the consumer's CONNECT state */
static void
consumer_connect(void)
{
  XDT_sdu sdu;

  get_sdu(&sdu);

  if (sdu.type == XDATind) {
    if (sdu.x.dat_ind.sequ == sequ) {
      conn = sdu.x.dat_ind.conn;
      write_data(sdu.x.dat_ind.data, sdu.x.dat_ind.length);
      ++sequ;

      state = DATA_TRANSFER;
    }
  }
}


/** @brief Implements the consumer's DATA_TRANSFER state */
static void
consumer_data_transfer(void)
{
  XDT_sdu sdu;

  get_sdu(&sdu);

  if (sdu.type == XDATind) {
    if (sdu.x.dat_ind.conn == conn && sdu.x.dat_ind.sequ == sequ) {
      write_data(sdu.x.dat_ind.data, sdu.x.dat_ind.length);
      ++sequ;
    }
  } else if (sdu.type == XABORTind) {
    if (sdu.x.abort_ind.conn == conn) {
      state = IDLE;
    }
  } else if (sdu.type == XDISind) {
    if (sdu.x.dis_ind.conn == conn) {
      state = IDLE;
    }
  }
}


/** 
 * @brief State scheduler
 *
 * Calls the appropriate function associated with the current service state.
 */
static void
run_consumer(void)
{
  do {
    switch (state) {
    case IDLE:
      consumer_idle();
      break;
    case CONNECT:
      consumer_connect();
      break;
    case DATA_TRANSFER:
      consumer_data_transfer();
      break;
    }
  } while (running);
}


/** 
 * @brief Consumer entry function
 *
 * After set up the environment this function is called to process 
 * the messages delivered by the XDT layer.
 *
 * The only functions needed here are
 * - get_sdu() to read SDU messages from the XDT layer,
 * - send_sdu() to send an SDU message to the XDT layer and
 * - write_data() to store the data received.
 */
void
start_consumer(void)
{
  run_consumer();
}


/**
 * @}
 */
