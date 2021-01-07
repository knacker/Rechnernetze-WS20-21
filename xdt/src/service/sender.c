/**
 * @file sender.c
 * @ingroup service
 * @brief XDT layer sender logic
 */

/**
 * @addtogroup service
 *  PR 02
 *  Vincent Schuster
 *  Maximilian Raspe
 */

#include "sender.h"
#include "service.h"
#include <stdlib.h>
#include <stdio.h>

/** @brief states of automata */
enum {
  IDLE,
  AWAIT_ACK,
  CONNECTED,
  GO_BACK_N,
  BREAK
};

/**  @brief timer message types */
enum {
  timer_msg_min_pred = pdu_msg_max_succ,
  T1,
  T2,
  T3,
  timer_msg_max_succ
};

/** @brief connection timer t1 t2 t3 */
static XDT_timer t1,t2,t3;

/** @brief Timeout constants t1 t2 t3*/
static double TIMEOUT1 = 5.;
static double TIMEOUT2 = 5.;
static double TIMEOUT3 = 10.;

/** @brief connection number of data transfer */
static unsigned conn = 0;

/** @brief current state */
static int state = IDLE;

/** @brief last visited state */
static int last_state = -1;

/** @brief last sequ */
static unsigned int last_sequ = 0;

/** @brief null pdu for buffer */
static XDT_pdu null;

/** @brief index for buffer */ 
static int buffer_index = -1;

/** @brief temporary index for buffer */
static int temp_index = -1;

/** @brief n from go_back_n */
#define n 5

/** @brief buffer, that saves n pdu DT */
static XDT_pdu buffer [n];

/** @brief sender running flag */
static int running = 1;

/** @brief shift not null elements left */
static void shift_buffer(void) {

  for (int i = 0; i < n; i++) {
    if (buffer[i].x.dt.sequ == 0) {
      if (i == (n-1)) break;
      buffer[i] = buffer[i+1];
      buffer[i+1] = null;
    }
  }
}

/** @brief implement sender's IDLE state */
static void sender_idle(void) {
  XDT_message msg;

  get_message(&msg);

  last_state = state;

  XDT_sdu* sdu;
  XDT_pdu pdu;

  if (msg.type == XDATrequ) {
      sdu = &msg.sdu;

      // create and send DT
      pdu.type = DT;
      pdu.x.dt.code = DT;
      pdu.x.dt.dest_addr = sdu->x.dat_requ.dest_addr;
      pdu.x.dt.source_addr = sdu->x.dat_requ.source_addr;
      pdu.x.dt.sequ = sdu->x.dat_requ.sequ;
      pdu.x.dt.eom = sdu->x.dat_requ.eom;
      XDT_COPY_DATA(&sdu->x.dat_requ.data,&pdu.x.dt.data,sdu->x.dat_requ.length);
      pdu.x.dt.length = sdu->x.dat_requ.length;

      send_pdu(&pdu);

      // start timer t1
      set_timer(&t1,TIMEOUT1);

      // change state
      state = AWAIT_ACK;
    }
}

/** @brief implement sender AWAIT_ACK state */
static void sender_await_ack(void) {
  XDT_message msg;

  get_message(&msg);

  last_state = state;

  XDT_sdu sdu;
  XDT_pdu* pdu;

  if (msg.type == ACK) {
    // reset timer t1
    reset_timer(&t1);
    
    pdu = &msg.pdu;

    conn = pdu->x.ack.conn;

    // if first ack received
    if (pdu->x.ack.sequ == 1) {
      // create and send XDATconf
      sdu.type = XDATconf;
      sdu.x.dat_conf.conn = conn;
      sdu.x.dat_conf.sequ = pdu->x.ack.sequ;
      send_sdu(&sdu);

      // start timers t2, t3
      set_timer(&t2,TIMEOUT2);
      set_timer(&t3,TIMEOUT3);

      state = CONNECTED;
    }

  // if timer t1 expired
  } else if (msg.type == T1) {
    // create and send XABORTind
    sdu.type = XABORTind;
    send_sdu(&sdu);

    // end program
    running = 0;
    state = IDLE;
  }
}

/** @brief implement sender CONNECTED state */
static void sender_connected(void) {
  XDT_message msg;

  get_message(&msg);

  XDT_sdu* sdu_recv;
  XDT_pdu* pdu_recv;

  XDT_sdu sdu_conf, sdu_abort_ind, sdu_break_ind, sdu_xdisind;
  XDT_pdu pdu;

  last_state = state;

  if (msg.type == ACK) {
      // reset and set timer t2
      reset_timer(&t2);
      set_timer(&t2, TIMEOUT2);

      pdu_recv = &msg.pdu;

      // check buffer
      for (int i = 0; i < n; i++) {
        // if received Ack belongs to DT in buffer delete DT
        if (buffer[i].x.dt.sequ == pdu_recv->x.ack.sequ) {
          //printf("                                       DT wurde bestaetigt, Index vorher: %d\n\n", buffer_index);
          buffer[i] = null;
          shift_buffer();
          buffer_index--;
          break;
        }
      }

      // if last ACK then state = IDLE and send_sdu(XDISind)
      if (pdu_recv->x.ack.sequ == last_sequ) {
        sdu_xdisind.type = XDISind;
        sdu_xdisind.x.dis_ind.conn = pdu_recv->x.ack.conn;

        send_sdu(&sdu_xdisind);

        running = 0;
        state = IDLE;
      }
      

    } else if (msg.type == ABO) {
      pdu_recv = &msg.pdu;

      // create and send XABORTind
      sdu_abort_ind.type = XABORTind;
      sdu_abort_ind.x.abort_ind.conn = pdu_recv->x.abo.conn;
      send_sdu(&sdu_abort_ind);
      running = 0;
      state = IDLE;

    } else if (msg.type == XDATrequ) {
      sdu_recv = &msg.sdu;
      
      conn = sdu_recv->x.dat_requ.conn;

      // create and send DT
      pdu.type = DT;
      pdu.x.dt.code = DT;
      pdu.x.dt.dest_addr = sdu_recv->x.dat_requ.dest_addr;
      pdu.x.dt.source_addr = sdu_recv->x.dat_requ.source_addr;
      pdu.x.dt.conn = conn;
      pdu.x.dt.sequ = sdu_recv->x.dat_requ.sequ;
      pdu.x.dt.eom = sdu_recv->x.dat_requ.eom;
      XDT_COPY_DATA(&sdu_recv->x.dat_requ.data,&pdu.x.dt.data,sdu_recv->x.dat_requ.length);
      pdu.x.dt.length = sdu_recv->x.dat_requ.length;

      send_pdu(&pdu);

      // save Copy of DT in buffer
      buffer_index++;
      buffer[buffer_index] = pdu;

      if (buffer_index == (n-1)) {
        // buffer is full -> send BREAKind
        state = BREAK;
        temp_index = buffer_index;
        sdu_break_ind.type = XBREAKind;
        sdu_break_ind.x.break_ind.conn = conn;

        // reset and set timer t2
        reset_timer(&t2);
        set_timer(&t2,TIMEOUT2);

        send_sdu(&sdu_break_ind);
      } else {

        // create and send XDATconf
        sdu_conf.type = XDATconf;
        sdu_conf.x.dat_conf.conn = sdu_recv->x.dat_requ.conn;
        sdu_conf.x.dat_conf.sequ = sdu_recv->x.dat_requ.sequ;
        send_sdu(&sdu_conf);
      }

      // reset and set timer t3
      reset_timer(&t3);
      set_timer(&t3,TIMEOUT3);

      // if last xdatrequ update last sequ
      if (sdu_recv->x.dat_requ.eom == 1) {
        last_sequ = sdu_recv->x.dat_requ.sequ;
      }

    } else if (msg.type == T2) {
      temp_index = buffer_index;
      state = GO_BACK_N;

    } else if (msg.type == T3) {
      sdu_abort_ind.type = XABORTind;
      send_sdu(&sdu_abort_ind);

      running = 0;
      state = IDLE;
    }
}

/** @brief implement sender GO_BACK_N state */
static void sender_go_back_n(void) {
  XDT_pdu pdu_go_back_n;

  pdu_go_back_n = buffer[n-temp_index-1];

  temp_index--;
  send_pdu(&pdu_go_back_n);

  // last elemenent
  if (temp_index == -1) {
    //reset and set t2
    reset_timer(&t2);
    set_timer(&t2,TIMEOUT2);
    if (last_state == BREAK) {
      state = BREAK;
    } else {
      state = CONNECTED;
    }
  }
}

/** @brief implement sender BREAK state */
static void sender_break(void) {
  XDT_message msg;

  get_message(&msg);

  XDT_sdu sdu;
  XDT_pdu* pdu;

  last_state = state;

  if (msg.type == ABO) {
      pdu = &msg.pdu;

      // create and send XABORTind
      sdu.type = XABORTind;
      sdu.x.abort_ind.conn = pdu->x.abo.conn;
      send_sdu(&sdu);
      running = 0;
      state = IDLE;

    } else if (msg.type == ACK) {

      pdu = &msg.pdu;

      // reset and set timers t2 and t3
      reset_timer(&t2);
      set_timer(&t2,TIMEOUT2);

      reset_timer(&t3);
      set_timer(&t3,TIMEOUT3);

      // check buffer
      for (int i = 0; i < n; i++) {
        if (buffer[i].x.dt.sequ == pdu->x.ack.sequ) {
          // printf("                                       DT wurde bestaetigt, Index vorher: %d\n\n", buffer_index);

          // if last ack form last element in buffer arrived -> send XDATconf to go on
          if (pdu->x.ack.sequ == buffer[buffer_index].x.dt.sequ) {
            sdu.type = XDATconf;
            sdu.x.dat_conf.conn = pdu->x.ack.conn;
            sdu.x.dat_conf.sequ = pdu->x.ack.sequ;
            send_sdu(&sdu);

            state = CONNECTED;
          }

          buffer[i] = null;
          shift_buffer();
          buffer_index--;

          break;
        }
      }

    } else if (msg.type == T2) {
      state = GO_BACK_N;

    } else if (msg.type == T3) {
      sdu.type = XABORTind;
      send_sdu(&sdu);

      running = 0;
      state = IDLE;
    }
}

static void
print_buffer(void) 
{
  printf("Bufferindex = %d\n",buffer_index);
  for (int i = 0; i < n; i++) {
    printf("Buffer Index %d : %u , %ld\n", i,buffer[i].x.dt.sequ,buffer[i].type);
  }
}

static void
init_buffer(void)
{
  // initialize buffer
  null.x.dt.sequ = 0;
  for (int i = 0; i < n; i++) {
    buffer[i] = null;
  }
}

/** 
 * @brief State scheduler
 *
 * Calls the appropriate function associated with the current protocol state.
 */
static void
run_sender(void)
{
  do {
    //printf("\nZustand Sender: %d\n",state);
    switch (state) {
    case (IDLE):
      sender_idle();
      break;

    case (AWAIT_ACK):
      sender_await_ack();
      break;

    case (CONNECTED):
      sender_connected();
      break;

    case (GO_BACK_N):
      sender_go_back_n();
      break;

    case (BREAK):
      sender_break();
      break;
    }
  } while (running);

} /* run_sender */

/** 
 * @brief Sender instance entry function
 *
 * After the dispatcher has set up a new sender instance      
 * and established a message queue between both processes  
 * this function is called to process the messages available    
 * in the message queue.        
 * The only functions and macros needed here are
 * - get_message() to read SDU, PDU and timer messages from the queue
 * - send_sdu() to send an SDU message to the producer,
 * - send_pdu() to send a PDU message to the receiving peer,
 * - #XDT_COPY_DATA to copy the message payload,	 
 * - create_timer() to create a timer associated with a message type,
 * - set_timer() to arm a timer (on expiration a timer associated message is
 *   put into the queue)
 * - reset_timer() to disarm a timer (all timer associated messages are removed
 *   from the queue)         
 * - delete_timer() to delete a timer.      
 */
void
start_sender(void)
{
  create_timer(&t1, T1);
  create_timer(&t2, T2);
  create_timer(&t3, T3);

  init_buffer();

  run_sender();

  delete_timer(&t1);
  delete_timer(&t2);
  delete_timer(&t3);
} /* start_sender */

/**
 * @}	
 */  