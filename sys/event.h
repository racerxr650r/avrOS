/**
 * @file event.h
 * @brief Event signaling and FSM wait/wake mechanism.
 *
 * Data types, macros, and function declarations to implement system events.
 *
 * Created: 7/13/2023
 * Author : john anderson
 *
 * Copyright (C) 2023 by John Anderson <racerxr650r@gmail.com>
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
#ifndef EVENT_H_
#define EVENT_H_

/** @addtogroup event_manager
 * @{
 */

// Includes -------------------------------------------------------------------
#include "../avrOS.h"

// Types ----------------------------------------------------------------------
typedef enum
{
	EVENT_ERROR = -1,
	EVENT_IDLE = 0,
	EVENT_ARMED,
	EVENT_DISARMED,
	EVENT_TRIGGERED
} evntState_t;

struct EVENT_DESCR_TYPE;
struct EVENT_TYPE;

typedef struct EVENT_STATS
{
	uint32_t	armed, triggered, disarmed, error;
}evntStats_t;

/**
 * @brief Event runtime status (RAM).
 *
 * Carries two integer fields that together implement the sub-type contract:
 *
 *   - evntType  - set by evntWait(sm, ev, type); the condition the consumer
 *                 is waiting for.
 *   - trigger   - set by evntTrigger(ev, subType); the condition the producer
 *                 is signaling.
 *
 * The default handler evntHandler() releases the waiting state machine only
 * when event->evntType == event->trigger, allowing one event object to
 * multiplex several sub-conditions without spurious wakeups.
 *
 * Sub-type 0 is reserved as the "armed but not yet triggered" sentinel; new
 * modules should number sub-types starting at 1. See doc/SDD.md sec. 4.3.4
 * for full conventions.
 */
typedef struct EVENT_TYPE
{
	evntState_t					state;
	volatile fsmStateMachine_t 		*stateMachine;
	int								evntType;		///< Condition consumer is waiting for (set by evntWait)
	int								trigger;		///< Condition producer signaled (set by evntTrigger)
	volatile struct EVENT_TYPE 		*next;
	const struct EVENT_DESCR_TYPE 	*descr;
#ifdef EVNT_STATS
	evntStats_t       				stats;
#endif
}event_t;

typedef int (*evntHandler_t)(volatile event_t *event);

typedef struct EVENT_DESCR_TYPE
{
	char    		*name;
	volatile event_t	*status;
	evntHandler_t	handler;
}evntDescriptor_t;

typedef struct EVENT_LIST
{
    volatile event_t *head;
    volatile event_t *tail;
    uint32_t size;
} evntList_t;

// External Functions ----------------------------------------------------------
static inline evntState_t evntGetStatus(volatile event_t *event)
{
	return(event->state);
}

static inline volatile fsmStateMachine_t* evntGetStateMachine()
{
	return(fsmGetCurrentStateMachine());
}

static inline int evntGetType(volatile event_t *event)
{
	return(event->evntType);
}

static inline int evntGetTrigger(volatile event_t *event)
{
	return(event->trigger);
}

volatile event_t* evntGetEvent(char *name);
evntState_t evntArm(volatile fsmStateMachine_t *stateMachine, volatile event_t *event);
evntState_t evntArmSystem(volatile event_t *event);
evntState_t evntDisarm(volatile event_t *event);
evntState_t evntTrigger(volatile event_t *event, int subType);
evntState_t evntWait(volatile fsmStateMachine_t *stateMachine, volatile event_t *event, int eventType);
int evntHandler(volatile event_t *event);
int evntInit(void);
int evntDispatch(void);

// Macros ----------------------------------------------------------------------
#ifdef EVNT_STATS
#define ADD_EVENT(evntName, ...)	\
		static volatile event_t	evntName; \
		const static evntDescriptor_t SECTION(EVNT_TABLE) CONCAT(evntName,_descr) = {.name = #evntName, .status = &evntName, .handler = DEFAULT_OR_ARG(,##__VA_ARGS__,__VA_ARGS__,evntHandler)}; \
		static volatile event_t	evntName = {.state = EVENT_DISARMED, .stateMachine = NULL, .descr = &CONCAT(evntName,_descr), .stats.armed = 0, .stats.disarmed = 0, .stats.triggered = 0, .stats.error = 0};
#else
#define ADD_EVENT(evntName, ...)	\
		static volatile event_t	evntName; \
		const static evntDescriptor_t SECTION(EVNT_TABLE) CONCAT(evntName,_descr) = {.name = #evntName, .status = &evntName, .handler = DEFAULT_OR_ARG(,##__VA_ARGS__,__VA_ARGS__,evntHandler)}; \
		static volatile event_t	evntName = {.state = EVENT_DISARMED, .stateMachine = NULL, .descr = &CONCAT(evntName,_descr)};
#endif

/** @} */ // end of event_manager

#endif  // EVENT_H_
