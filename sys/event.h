/*
 * event.h
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

// Includes -------------------------------------------------------------------
#include "../avrOS.h"

// Types ----------------------------------------------------------------------
typedef enum
{
	EVENT_ERROR = -1,
	EVENT_NONE = 0,
	EVENT_ARMED,
	EVENT_DISARMED,
	EVENT_TRIGGERED
} evntCode_t;

typedef int (*evntHandler_t)(volatile fsmStateMachine_t *stateMachine);

struct EVENT_TYPE;
struct EVENT_STATE_TYPE;

typedef struct EVENT_STATS
{
	uint32_t	triggered, error;
}evntStats_t;

typedef struct EVENT_STATE_TYPE
{
	int8_t            signal, trigger;
	volatile fsmStateMachine_t *stateMachine;
	evntHandler_t     handler;
	const struct EVENT_TYPE *descr;
#ifdef EVNT_STATS
	evntStats_t       stats;
#endif
}event_t;

typedef struct EVENT_TYPE
{
	char    *name;
	volatile event_t *status;
}evntDescriptor_t;

// Macros ----------------------------------------------------------------------
#ifdef EVNT_STATS
#define ADD_EVENT(evntName)	\
        volatile static event_t	evntName; \
	    const static evntDescriptor_t SECTION(EVNT_TABLE) CONCAT(evntName,_descr) = {.name = #evntName, .status = &evntName}; \
        volatile static event_t	evntName = {.signal = EVENT_NONE, .trigger = EVENT_NONE, .stateMachine = NULL, .handler = NULL, .descr = &CONCAT(evntName,_descr), .stats.triggered = 0, .stats.error = 0};
#else
#define ADD_EVENT(evntName)	\
        volatile static event_t	evntName; \
	    const static evntDescriptor_t SECTION(EVNT_TABLE) CONCAT(evntName,_descr) = {.name = #evntName, .status = &evntName}; \
        volatile static event_t	evntName = {.signal = EVENT_NONE, .trigger = EVENT_NONE, .stateMachine = NULL, .handler = NULL, .descr = &CONCAT(evntName,_descr)};
#endif

#define evntGetSignal(event)        (event->signal)
#define evntSetSignal(event,signal) event->signal = signal;

// Inline functions ------------------------------------------------------------
inline const char* evntGetName(volatile event_t *event)
{
	return(event->descr->name);
}

// External Functions ----------------------------------------------------------
extern volatile event_t* evntGetEvent(char *name);
extern int evntReset(volatile event_t *event);
extern int evntEnable(volatile event_t *event, int8_t trigger, evntHandler_t handler, volatile fsmStateMachine_t *stateMachine);
extern int evntDisable(volatile event_t *event);
extern int evntTrigger(volatile event_t *event, int8_t trigger);
extern int evntDispatch(void);

#endif  // EVENT_H_
