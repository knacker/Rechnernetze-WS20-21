/**
 * @file timer.h
 * @ingroup xdt
 * @brief XDT real-time timer wrapper
 */

#ifndef TIMER_H
#define TIMER_H

/**
 * @example timers.c
 * Demonstration of how the XDT timer implementation may be used.
 */

/**
 * @addtogroup xdt
 * @{
 */


#ifndef _POSIX_C_SOURCE
/** @brief Make all following header specific symbols required by IEEE Std 1003.1-2001 (SUSv3) to appear */
#define _POSIX_C_SOURCE 200112L
#endif

#include <signal.h>
#include <time.h>

/**
 * @brief Base signal number
 *
 * Use this and successive signal numbers with xdt_timer_create() calls.
 */
#define TIMER_SIGNAL_BASE SIGRTMIN

/**
 * @brief XDT timer context
 *
 * Holds timer context information (use as an opaque type).
 */
typedef struct
{
  timer_t id;
  int type;
} XDT_timer;


/**
 * @brief Timout handler funtion
 *
 * Type of an signal handler function, called when a timer expires.
 *
 * @param signo Contains the signal number (#TIMER_SIGNAL_BASE or higher) raised by the expired timer.
 * @param info Signal information, see @e sigaction(2). @a info->si_value.sival_int will contain 
 *        the timers @e type given in the appropriate xdt_timer_create() call.
 * @param cruft Not used.
 *
 * @warning Do not make any assumptions about the @a cruft parameter nor dereference the (casted) pointer.
 */
typedef void (*timeout_handler_func) (int signo, siginfo_t * info, void *cruft);

int xdt_timer_create(XDT_timer * timer, int signo, timeout_handler_func handler, int type);
int xdt_timer_set(XDT_timer * timer, double timeout);
int xdt_timer_delete(XDT_timer * timer);

/** 
 * @brief Resets an XDT timer
 *
 * Convenient macro to xdt_timer_set().
 * 
 * @param timer points to an XDT timer object
 */
#define xdt_timer_reset(timer) xdt_timer_set(timer, -1.0)


/**
 * @}
 */

#endif /* TIMER_H */
