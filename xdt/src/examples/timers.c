/* timers.c
 *
 * cc -pedantic -W -Wall -I../xdt -o timer_test timer_test.c ../xdt/timer.c [-lrt|-lposix]
 *
 * This example demonstrates the various ways, how to use the XDT timer functions.
 * (Note: It is a mess in showing what NOT to do in signal handlers: not calling stdio functions!)
 *
 * usage: ./timers
 */

#include <xdt/timer.h>

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>


#define T1_SIGNAL TIMER_SIGNAL_BASE
#define T1_HANDLER shared_timeout_handler
#define T1_TYPE 1
#define T1_TIMEOUT 10.0

#define T2_SIGNAL TIMER_SIGNAL_BASE
#define T2_HANDLER shared_timeout_handler
#define T2_TYPE 2
#define T2_TIMEOUT 5.0

#define T3_SIGNAL TIMER_SIGNAL_BASE+1
#define T3_HANDLER shared_timeout_handler
#define T3_TYPE 3
#define T3_TIMEOUT 15.0

#define T4_SIGNAL TIMER_SIGNAL_BASE+2
#define T4_HANDLER shared_timeout_handler
#define T4_TYPE 4
#define T4_TIMEOUT 20.0

#define T5_SIGNAL TIMER_SIGNAL_BASE+3
#define T5_HANDLER exclusive_timeout_handler
#define T5_TYPE 5
#define T5_TIMEOUT 25.0


static XDT_timer t1, t2, t3, t4, t5;
static volatile sig_atomic_t should_quit = 0;


static void
shared_timeout_handler(int signo, siginfo_t * info, void *cruft)
{
  /* real time signals are not necessarily constants
   * so we can't use these as case labels */

  if (signo == T1_SIGNAL) {
    puts("T1_SIGNAL raised");

  } else if (signo == T2_SIGNAL) {
    puts("T2_SIGNAL raised");

  } else if (signo == T3_SIGNAL) {
    puts("T3_SIGNAL raised");

  } else if (signo == T4_SIGNAL) {
    puts("T4_SIGNAL raised");
  }

  /* since you can use the same signal for different timers
   * you might consider to distinguish timers by their 
   * values and use these instead */

  switch (info->si_value.sival_int) {

  case T1_TYPE:
    puts("timer 1 expired");
    break;

  case T2_TYPE:
    puts("timer 2 expired");
    break;

  case T3_TYPE:
    puts("timer 3 expired");
    if (xdt_timer_reset(&t4) < 0) {
      perror("");
      exit(44);
    }
    puts("timer 4 disarmed");
    break;

  case T4_TYPE:
    puts("timer 4 expired");
    break;
  }

  /* avoid 'unused parameter' compiler warning */
  cruft = 0;
}

static void
exclusive_timeout_handler(int signo, siginfo_t * info, void *cruft)
{
  /* another method is to use a separate handler for each timer
   * and don't care about signals and timer values */

  puts("timer 5 expired");
  should_quit = 1;

  /* avoid 'unused parameter' compiler warnings */
  signo = signo;
  info = 0;
  cruft = 0;
}


int
main(void)
{
  time_t now;

  /* create all timers */
  if (xdt_timer_create(&t1, T1_SIGNAL, T1_HANDLER, T1_TYPE) < 0) {
    perror("");
    exit(11);
  }
  if (xdt_timer_create(&t2, T2_SIGNAL, T2_HANDLER, T2_TYPE) < 0) {
    perror("");
    exit(12);
  }
  if (xdt_timer_create(&t3, T3_SIGNAL, T3_HANDLER, T3_TYPE) < 0) {
    perror("");
    exit(13);
  }
  if (xdt_timer_create(&t4, T4_SIGNAL, T4_HANDLER, T4_TYPE) < 0) {
    perror("");
    exit(14);
  }
  if (xdt_timer_create(&t5, T5_SIGNAL, T5_HANDLER, T5_TYPE) < 0) {
    perror("");
    exit(15);
  }

  /* set all timers */
  if (xdt_timer_set(&t1, T1_TIMEOUT) < 0) {
    perror("");
    exit(21);
  }
  if (xdt_timer_set(&t2, T2_TIMEOUT) < 0) {
    perror("");
    exit(22);
  }
  if (xdt_timer_set(&t3, T3_TIMEOUT) < 0) {
    perror("");
    exit(23);
  }
  if (xdt_timer_set(&t4, T4_TIMEOUT) < 0) {
    perror("");
    exit(24);
  }
  if (xdt_timer_set(&t5, T5_TIMEOUT) < 0) {
    perror("");
    exit(25);
  }

  time(&now);
  puts(ctime(&now));

  /* wait for timers to expire */
  while (!should_quit) {
    pause();
    time(&now);
    puts(ctime(&now));
  }

  return 0;
}
