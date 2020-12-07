/**
 * @file timer.c
 * @ingroup xdt
 * @brief XDT real-time timer wrapper
 */

/**
 * @addtogroup xdt
 * @{
 */

#include "timer.h"

#include <errno.h>

/**
 * @brief Creates an XDT timer
 *
 * @param timer points to an XDT timer object
 * @param signo signal to raise by this timer (must be #TIMER_SIGNAL_BASE or higher)
 * @param handler signal handler called when timer expires
 * @param type internal timer message type
 *
 * @return 0 on sucess, value < 0 on error
 */
int
xdt_timer_create(XDT_timer * timer, int signo, timeout_handler_func handler, int type)
{
  struct sigaction sa;
  struct sigevent sev;

  errno = 0;

  if (!timer || signo < TIMER_SIGNAL_BASE) {
    return -10;
  }

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;
#ifdef SA_RESTART
  /* restart system calls after interruption by timer signal */
  sa.sa_flags = SA_RESTART;
#endif
  sa.sa_sigaction = handler;
  if (sigaction(signo, &sa, 0) == -1) {
    return -15;
  }

  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo = signo;
  sev.sigev_value.sival_int = type;
  if (timer_create(CLOCK_REALTIME, &sev, &timer->id) == -1) {
    return -1;
  }

  timer->type = type;

  return 0;
}


/**
 * @brief Re-sets a relative timer
 *
 * @param timer timer to set
 * @param timeout relative time in seconds after the timer expires
 *        (zero or negative value will disarm the timer)
 *
 * @return 0 on sucess, value < 0 on error
 */
int
xdt_timer_set(XDT_timer * timer, double timeout)
{
  struct itimerspec spec;

  errno = 0;

  if (!timer) {
    return -10;
  }

  if (timeout <= 0.0) {
    spec.it_interval.tv_sec = 0;
    spec.it_interval.tv_nsec = 0;
    spec.it_value.tv_sec = 0;
    spec.it_value.tv_nsec = 0;
  } else {
    spec.it_interval.tv_sec = 0;
    spec.it_interval.tv_nsec = 0;
    spec.it_value.tv_sec = (time_t) timeout;
    spec.it_value.tv_nsec = (timeout - (long)timeout) * 1000000000L;
  }

  if (timer_settime(timer->id, 0, &spec, 0) == -1) {
    return -1;
  }

  return 0;
}


/**
 * @brief Deletes an XDT timer
 *
 * @param timer timer to delete, must be created before by xdt_timer_create()
 * 
 * @return 0 on success, value < 0 on error
 */
int
xdt_timer_delete(XDT_timer * timer)
{
  if (!timer) {
    return -10;
  }

  return timer_delete(timer->id);
}

/**
 * @}
 */
