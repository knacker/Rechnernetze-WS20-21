/**
 * @file receiver.c
 * @ingroup service
 * @brief XDT layer receiver logic
 */

/**
 * @addtogroup service
 *  PR 02
 *  Vincent Schuster
 *  Maximilian Raspe
 */

#include "receiver.h"
#include "service.h"
#include <stdlib.h>
#include <stdio.h>

/** @brief states of automata */
enum {
    IDLE,
    CONNECTED,
    AWAIT_CORRECT_DT
};

/** @brief connection number of data transfer */
static unsigned conn = 0;

/** @brief sequ number of data transfer */
static unsigned sequ = 0;

/** @brief connection timer */
static XDT_timer timer;

/** @brief Timeout constant */
static double TIMEOUT = 10.;

/** @brief sender running flag */
static int running = 1;

/** @brief current state */
static int state = IDLE;

/** @brief timer message type */
enum {
    timer_msg_min_pred = pdu_msg_max_succ,
    TI,
    timer_msg_max_succ
};

static void receiver_idle(void) 
{
  XDT_message msg;
  XDT_pdu* pdu_dt;
  XDT_pdu pdu_ack;
  XDT_sdu sdu;

  get_message(&msg);

  if (msg.type == DT) {
    pdu_dt = &msg.pdu;

    // if first DT received
    if (pdu_dt->x.dt.sequ == 1) {
      
      // update sequ
      sequ = pdu_dt->x.dt.sequ;

      // create and send XDATind
      sdu.type = XDATind;
      sdu.x.dat_ind.conn = conn;
      sdu.x.dat_ind.sequ = pdu_dt->x.dt.sequ;
      sdu.x.dat_ind.eom = pdu_dt->x.dt.eom;
      sdu.x.dat_ind.length = pdu_dt->x.dt.length;
      XDT_COPY_DATA(&pdu_dt->x.dt.data, &sdu.x.dat_ind.data, pdu_dt->x.dt.length);

      send_sdu(&sdu);

      // create and send ACK
      pdu_ack.type = ACK;
      pdu_ack.x.ack.code = ACK;
      pdu_ack.x.ack.source_addr = pdu_dt->x.dt.dest_addr;
      pdu_ack.x.ack.dest_addr = pdu_dt->x.dt.source_addr;
      pdu_ack.x.ack.conn = conn;
      pdu_ack.x.ack.sequ = pdu_dt->x.dt.sequ;

      send_pdu(&pdu_ack);

      // start timer
      set_timer(&timer, TIMEOUT);

      state = CONNECTED;
    }
  }
}

static void receiver_connect(void) {
  XDT_message msg;
  XDT_pdu* pdu;
  XDT_sdu sdu;
  XDT_pdu pdu_send;

  get_message(&msg);

  if (msg.type == DT)
  {
    pdu = &msg.pdu;

    // reset timer
    reset_timer(&timer);
    set_timer(&timer,TIMEOUT);

    conn = pdu->x.dt.conn;

    // if last package arrived
    if (pdu->x.dt.eom == 1) {

      // create and send ACK
      pdu_send.type = ACK;
      pdu_send.x.ack.code = ACK;
      pdu_send.x.ack.conn = conn;
      pdu_send.x.ack.dest_addr = pdu->x.dt.source_addr;
      pdu_send.x.ack.source_addr = pdu->x.dt.dest_addr;
      pdu_send.x.ack.sequ = pdu->x.dt.sequ;

      send_pdu(&pdu_send);

      // create and send XDATind
      sdu.type = XDATind;
      sdu.x.dat_ind.conn = conn;
      sdu.x.dat_ind.sequ = pdu->x.dt.sequ;
      sdu.x.dat_ind.eom = pdu->x.dt.eom;
      sdu.x.dat_ind.length = pdu->x.dt.length;
      XDT_COPY_DATA(&pdu->x.dt.data, &sdu.x.dat_ind.data, pdu->x.dt.length);

      send_sdu(&sdu);

      // create and send XDIsind
      sdu.type = XDISind;
      sdu.x.dis_ind.conn = pdu->x.dt.conn;

      send_sdu(&sdu);

      running = 0;
      state = IDLE;

    } else {

      // if wrong sequ
      if (pdu->x.dt.sequ != (sequ + 1)) {
        state = AWAIT_CORRECT_DT;
      } else {

        // valid sequ received
        sequ = pdu->x.dt.sequ;

        // create and send XDATind
        sdu.type = XDATind;
        sdu.x.dat_ind.conn = conn;
        sdu.x.dat_ind.sequ = pdu->x.dt.sequ;
        sdu.x.dat_ind.eom = pdu->x.dt.eom;
        sdu.x.dat_ind.length = pdu->x.dt.length;
        XDT_COPY_DATA(&pdu->x.dt.data, &sdu.x.dat_ind.data, pdu->x.dt.length);

        send_sdu(&sdu);

        // create and send ACK
        pdu_send.type = ACK;
        pdu_send.x.ack.code = ACK;
        pdu_send.x.ack.source_addr = pdu->x.dt.dest_addr;
        pdu_send.x.ack.dest_addr = pdu->x.dt.source_addr;
        pdu_send.x.ack.conn = conn;
        pdu_send.x.ack.sequ = pdu->x.dt.sequ;

        send_pdu(&pdu_send);
      }
    }

  // if timer expired
  } else if (msg.type == TI) {
    // create and send ABO
    pdu_send.type = ABO;
    pdu_send.x.abo.code = ABO;
    pdu_send.x.abo.conn = conn;
    send_pdu(&pdu_send);

    // create and send ABORTInd
    sdu.type = XABORTind;
    send_sdu(&sdu);

    running = 0;
    state = IDLE;
  }
}

static void
receiver_await_correct_dt(void)
{

  XDT_message msg;
  XDT_pdu *pdu;
  XDT_pdu pdu_send;
  XDT_sdu sdu;

  get_message(&msg);

    // if timer expired
    if(msg.type == TI) {
      
      // create and send ABO
      pdu_send.type = ABO;
      pdu_send.x.abo.code = ABO;
      pdu_send.x.abo.conn = conn;
      send_pdu(&pdu_send);

      // create and send XABORTind
      sdu.type = XABORTind;
      sdu.x.abort_ind.conn = conn;
      send_sdu(&sdu);

      state = IDLE;
      running = 0;

    } else if (msg.type == DT) {

      pdu = &msg.pdu;

      conn = pdu->x.dt.conn;

      // reset timer
      reset_timer(&timer);
      set_timer(&timer,TIMEOUT);

      // if last package arrived
      if (pdu->x.dt.eom == 1) {

        // create and send ACK
        pdu_send.type = ACK;
        pdu_send.x.ack.code = pdu->x.dt.code;
        pdu_send.x.ack.conn = pdu->x.dt.conn;
        pdu_send.x.ack.dest_addr = pdu->x.dt.source_addr;
        pdu_send.x.ack.source_addr = pdu->x.dt.dest_addr;
        pdu_send.x.ack.sequ = pdu->x.dt.sequ;

        send_pdu(&pdu_send);

        // create and send XDATind
        sdu.type = XDATind;
        sdu.x.dat_ind.conn = conn;
        sdu.x.dat_ind.sequ = pdu->x.dt.sequ;
        sdu.x.dat_ind.eom = pdu->x.dt.eom;
        sdu.x.dat_ind.length = pdu->x.dt.length;
        XDT_COPY_DATA(&pdu->x.dt.data, &sdu.x.dat_ind.data, pdu->x.dt.length);

        send_sdu(&sdu);

        // create XDIsind
        sdu.type = XDISind;
        sdu.x.dis_ind.conn = pdu->x.dt.conn;

        send_sdu(&sdu);

        running = 0;
        state = IDLE;

      } else {

        // if wrong sequ
        if (pdu->x.dt.sequ != (sequ + 1)) {
          state = AWAIT_CORRECT_DT;
        } else {
          // valid sequ received

          // create and send XDATind
          sdu.type = XDATind;
          sdu.x.dat_ind.conn = conn;
          sdu.x.dat_ind.sequ = pdu->x.dt.sequ;
          sdu.x.dat_ind.eom = pdu->x.dt.eom;
          sdu.x.dat_ind.length = pdu->x.dt.length;
          XDT_COPY_DATA(&pdu->x.dt.data, &sdu.x.dat_ind.data, pdu->x.dt.length);

          send_sdu(&sdu);

          // create and send ACK
          pdu_send.type = ACK;
          pdu_send.x.ack.code = ACK;
          pdu_send.x.ack.source_addr = pdu->x.dt.dest_addr;
          pdu_send.x.ack.dest_addr = pdu->x.dt.source_addr;
          pdu_send.x.ack.conn = conn;
          pdu_send.x.ack.sequ = pdu->x.dt.sequ;

          send_pdu(&pdu_send);

          //update sequ
          sequ = pdu->x.dt.sequ;

          // reset and set timer
          reset_timer(&timer);
          set_timer(&timer,TIMEOUT);

          state = CONNECTED;
        }
      }
  }
}

/** 
 * @brief State scheduler
 *
 * Calls the appropriate function associated with the current protocol state.
 */
static void
run_receiver(void)
{
  while(running != 0) {

    //printf("\nZustand Receiver: %d\n",state);

    if(state == IDLE) {
      receiver_idle();
    }
    else if(state == AWAIT_CORRECT_DT) {
      receiver_await_correct_dt();
    }
    else if(state == CONNECTED) {
      receiver_connect();
    }
  }
} /* run_receiver */

/** 
 * @brief Receiver instance entry function
 *
 * After the dispatcher has set up a new receiver instance
 * and established a message queue between both processes
 * this function is called to process the messages available
 * in the message queue.
 * The only functions and macros needed here are        
 * - get_message() to read SDU, PDU and timer messages from the queue
 * - send_sdu() to send an SDU message to the consumer,     
 * - send_pdu() to send a PDU message to the sending peer,
 * - #XDT_COPY_DATA to copy the message payload,        
 * - create_timer() to create a timer associated with a message type,
 * - set_timer() to arm a timer (on expiration a timer associated message is  
 *   put into the queue)                    
 * - reset_timer() to disarm a timer (all timer associated messages are removed  
 *   from the queue)
 * - delete_timer() to delete a timer.
 *
 * @param connection the connection number assigned to the data transfer
 *        handled by this instance
 */
void
start_receiver(unsigned connection)
{
  conn = connection;
  create_timer(&timer, TI);
  run_receiver();
  delete_timer(&timer);

} /* start_receiver */


/**
 * @}
 */
