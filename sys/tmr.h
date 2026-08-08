/**
 * @file tmr.h
 * @brief Precision Timer — 32-bit RTC+TCB cascade timers with FSM/event wake.
 *
 * Declares the data types, registration macros, and API for the precision
 * timer subsystem (see doc/SDD.md sec. 4.5). Each timer measures a delay with
 * ~1024 ticks/sec precision by default, built on a 32-bit hardware counter
 * formed by cascading the Real-Time Counter (RTC, low 16 bits) with a
 * Timer/Counter type B (TCB, high 16 bits) through the peripheral event system
 * (EVSYS). When a timer expires it signals an associated avrOS event which can
 * either wake a waiting state machine (via tmrWait) or invoke a user callback
 * in dispatch context (ADD_TMR) or interrupt context (ADD_TMR_ISR).
 *
 * The scheduler keeps a list of active timers ordered by expiry and programs a
 * two-stage compare interrupt (coarse TCB overflow count, then fine RTC
 * compare) for the next timer to expire. Hardware selection (TCB instance, RTC
 * clock source, prescaler, EVSYS channel) is configured in avrOSConfig.h.
 *
 * Created: 7/3/2026
 * Author: john anderson
 *
 * Copyright (C) 2021 by John Anderson <racerxr650r@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef TMR_H_
#define TMR_H_

/** @addtogroup precision_timer
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include "event.h"
#include "fsm.h"

// Data Types -----------------------------------------------------------------
/**
 * @brief Precision timer event sub-types.
 *
 * Registered on the timer's associated event object (see doc/SDD.md sec.
 * 4.3.4). Sub-type 0 is reserved as the "armed but not yet triggered"
 * sentinel, so timer sub-types start at 1.
 */
typedef enum
{
	TMR_EVENT_EXPIRED = 1	///< The timer counted down to zero.
} tmrEvents_t;

// Forward declarations
struct TIMER_DESCR_TYPE;

/**
 * @brief Precision timer runtime state (RAM).
 *
 * One instance per timer, declared by ADD_TMR / ADD_TMR_ISR. The scheduler
 * stores each timer's expiry as an absolute value on the module's software
 * tick clock; the ticks remaining at any moment are derived from it (see
 * tmrGet). @c duration retains the original tick count passed to tmrSet.
 */
typedef struct TIMER_TYPE
{
	uint32_t						duration;	///< Original tick count from tmrSet().
	uint32_t						expiry;		///< Absolute expiry time on the module tick clock.
	bool							active;		///< True while on the active (counting) list.
	volatile struct TIMER_TYPE		*next;		///< Intrusive active-list link (expiry order).
	const struct TIMER_DESCR_TYPE	*descr;		///< Pointer to the flash-resident descriptor.
} timer_t;

/**
 * @brief Precision timer descriptor (flash).
 *
 * One instance per ADD_TMR / ADD_TMR_ISR, placed in the TMR_TABLE linker
 * section and walked by the CLI command.
 */
typedef struct TIMER_DESCR_TYPE
{
	const char			*name;	///< Human-readable name used by the CLI.
	volatile timer_t	*timer;	///< Pointer to the RAM-resident timer_t.
	volatile event_t	*event;	///< Associated event signaled on expiry.
} timerDescr_t;

// Registration Macros --------------------------------------------------------
/**
 * @brief Declare, register, and initialize a precision timer.
 *
 * Allocates the RAM @ref timer_t, emits the flash @ref timerDescr_t in the
 * TMR_TABLE linker section, and creates the timer's associated event (via
 * @ref ADD_EVENT) — all in a single declaration. The event uses the default
 * handler (waking any FSM that called tmrWait) unless an optional
 * dispatch-context @ref evntHandler_t is supplied.
 *
 * Usage:
 * @code
 *   ADD_TMR(blink);                 // fires blink_evnt on expiry
 *   ADD_TMR(blink, myDispatchCb);   // custom dispatch-context callback
 * @endcode
 *
 * @param tmrName  Token used as the C variable name for the timer_t.
 * @param ...      Optional dispatch-context @ref evntHandler_t.
 */
#define ADD_TMR(tmrName, ...) \
		const static timerDescr_t CONCAT(tmrName,_descr); \
		static volatile timer_t tmrName = {.duration = 0, .expiry = 0, .active = false, .next = NULL, .descr = &CONCAT(tmrName,_descr)}; \
		ADD_EVENT(tmrName ## _evnt, ##__VA_ARGS__); \
		const static timerDescr_t SECTION(TMR_TABLE) CONCAT(tmrName,_descr) = {.name = #tmrName, .timer = &tmrName, .event = &CONCAT(tmrName,_evnt)};

/**
 * @brief Declare a precision timer with an interrupt-context expiry callback.
 *
 * Identical to @ref ADD_TMR but registers @p handler (type
 * @ref evntIsrHandler_t) as a direct ISR-context callback via
 * @ref ADD_EVENT_ISR. On expiry the callback runs immediately in the timer
 * interrupt, giving least jitter, and is not queued for dispatch. A single
 * declaration — no second macro is required.
 *
 * Usage:
 * @code
 *   ADD_TMR_ISR(pulse, pulseIsr);   // pulseIsr runs in the expiry ISR
 * @endcode
 *
 * @param tmrName  Token used as the C variable name for the timer_t.
 * @param handler  ISR-context @ref evntIsrHandler_t callback.
 */
#define ADD_TMR_ISR(tmrName, handler) \
		const static timerDescr_t CONCAT(tmrName,_descr); \
		static volatile timer_t tmrName = {.duration = 0, .expiry = 0, .active = false, .next = NULL, .descr = &CONCAT(tmrName,_descr)}; \
		ADD_EVENT_ISR(tmrName ## _evnt, handler); \
		const static timerDescr_t SECTION(TMR_TABLE) CONCAT(tmrName,_descr) = {.name = #tmrName, .timer = &tmrName, .event = &CONCAT(tmrName,_evnt)};

// API Macros -----------------------------------------------------------------
/**
 * @brief Arm (or re-arm) a timer to expire after @p ticks ticks.
 * @param tmrName  Timer declared with ADD_TMR / ADD_TMR_ISR.
 * @param ticks    Delay in timer ticks (1024/sec by default). Must be in the
 *                 range 1 .. 0x7FFFFFFF (about 24 days at the default rate).
 * @return true if the timer was armed, false on invalid argument.
 */
#define tmrSet(tmrName, ticks)			tmrSetTimer(&(tmrName), (ticks))

/**
 * @brief Return the ticks remaining before a timer expires.
 * @param tmrName  Timer declared with ADD_TMR / ADD_TMR_ISR.
 * @return Ticks remaining, or 0 if the timer is inactive/expired.
 */
#define tmrGet(tmrName)					tmrGetRemaining(&(tmrName))

/**
 * @brief Cancel a running timer and disarm its event.
 * @param tmrName  Timer declared with ADD_TMR / ADD_TMR_ISR.
 * @return true if the timer was cancelled, false on invalid argument.
 */
#define tmrCancel(tmrName)				tmrCancelTimer(&(tmrName))

/**
 * @brief Suspend the calling FSM until the timer expires.
 *
 * Convenience wrapper around evntWait() on the timer's associated event using
 * the @ref TMR_EVENT_EXPIRED sub-type. Arm the timer with tmrSet() first.
 *
 * @param tmrName      Timer declared with ADD_TMR (dispatch-context).
 * @param resumeState  FSM state handler to resume when the timer expires.
 * @return Resulting event state (see evntWait()).
 */
#define tmrWait(tmrName, resumeState)	evntWait(&CONCAT(tmrName,_evnt), TMR_EVENT_EXPIRED, (resumeState))

// External Functions ---------------------------------------------------------
/**
 * @brief Arm or re-arm a timer (backing function for @ref tmrSet).
 *
 * @param tmr    Pointer to the timer to arm.
 * @param ticks  Delay in timer ticks; must be > 0.
 * @return true if armed, false on invalid argument.
 */
bool tmrSetTimer(volatile timer_t *tmr, uint32_t ticks);

/**
 * @brief Return the ticks remaining (backing function for @ref tmrGet).
 *
 * @param tmr  Pointer to the timer.
 * @return Ticks remaining, or 0 if inactive/expired.
 */
uint32_t tmrGetRemaining(volatile timer_t *tmr);

/**
 * @brief Cancel a timer (backing function for @ref tmrCancel).
 *
 * @param tmr  Pointer to the timer.
 * @return true if cancelled, false on invalid argument.
 */
bool tmrCancelTimer(volatile timer_t *tmr);

/** @} */ // end of precision_timer

#endif /* TMR_H_ */
