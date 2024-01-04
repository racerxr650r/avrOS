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
	EVENT_IDLE = 0,
	EVENT_ARMED,
	EVENT_DISARMED,
	EVENT_TRIGGERED
} evntState_t;

typedef enum
{
	EVENT_TYPE_ALL = 0,
	EVENT_TYPE_NONE = 0,
	EVENT_TYPE_1 = 0b00000001,
	EVENT_TYPE_2 = 0b00000010,
	EVENT_TYPE_3 = 0b00000100,
	EVENT_TYPE_4 = 0b00001000,
	EVENT_TYPE_5 = 0b00010000,
	EVENT_TYPE_6 = 0b00100000,
	EVENT_TYPE_7 = 0b01000000,
	EVENT_TYPE_8 = 0b10000000,
} evntType_t;

typedef int (*evntHandler_t)(volatile fsmStateMachine_t *stateMachine);

struct EVENT_DESCR_TYPE;
struct EVENT_TYPE;

typedef struct EVENT_STATS
{
	uint32_t	armed, handled, unhandled, error;
}evntStats_t;

typedef struct EVENT_TYPE
{
	evntType_t        				type, filter;
	volatile fsmStateMachine_t 		*stateMachine;
	evntHandler_t     				handler;
#ifdef EVNT_STATS
	const struct EVENT_DESCR_TYPE 	*descr;
	evntStats_t       	stats;
#endif
}event_t;

typedef struct EVENT_DESCR_TYPE
{
	char    			*name;
	volatile event_t	*status;
}evntDescriptor_t;

// Macros ----------------------------------------------------------------------
#ifdef EVNT_STATS
#define ADD_EVENT(evntName)	\
		volatile static event_t	evntName; \
		const static evntDescriptor_t SECTION(EVNT_TABLE) CONCAT(evntName,_descr) = {.name = #evntName, .status = &evntName}; \
		volatile static event_t	evntName = {.type = EVENT_TYPE_NONE, .filter = EVENT_TYPE_ALL, .stateMachine = NULL, .handler = NULL, .descr = &CONCAT(evntName,_descr), .stats.armed = 0, .stats.handled = 0, .stats.unhandled = 0, .stats.error = 0};
#else
#define ADD_EVENT(evntName)	\
		volatile static event_t	evntName; \
		volatile static event_t	evntName = {.type = EVENT_TYPE_NONE, .filter = EVENT_TYPE_ALL, .stateMachine = NULL, .handler = NULL};
#endif

// External Functions ----------------------------------------------------------
volatile event_t* evntGetEvent(char *name);
evntState_t evntReset(volatile event_t *event);
evntState_t evntEnable(volatile event_t *event, evntType_t filter, evntHandler_t handler, volatile fsmStateMachine_t *stateMachine);
evntState_t evntWait(volatile event_t *event, evntType_t filter);
evntState_t evntDisable(volatile event_t *event);
evntState_t evntTrigger(volatile event_t *event, evntType_t type);
int evntDispatch(void);

#endif  // EVENT_H_
