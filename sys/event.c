/*
 * event.c
 *
 * Functions to implement system events.
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

#include "avrOS.h"

// Externs ---------------------------------------------------------------------
extern void *__start_EVNT_TABLE,*__stop_EVNT_TABLE;

// Locals ----------------------------------------------------------------------
evntList_t	evntListArmed = { .head = NULL, .tail = NULL, .size = 0 };
evntList_t	evntListDisarmed = { .head = NULL, .tail = NULL, .size = 0 };
evntList_t	evntListTriggered = { .head = NULL, .tail = NULL, .size = 0 };

// Command line interface ------------------------------------------------------
#ifdef EVNT_CLI
ADD_COMMAND("evnt",evntCmd,true);
#endif

static osStatus_t evntCmd(int argc, char *argv[])
{
	evntDescriptor_t    *descr = (evntDescriptor_t *)&__start_EVNT_TABLE;
	osStatus_t          ret = OS_ERROR;

	// Walk the table of events
	for(; descr < (evntDescriptor_t *)&__stop_EVNT_TABLE; ++descr)
	{
		if(argc<2 || (argc==2 && !strcmp(descr->name,argv[1])))
		{
#ifdef EVNT_STATS
			printf(UNDERLINE BOLD FG_BLUE "%-24s",descr->name);
#endif
			printf(UNDERLINE BOLD FG_BLUE "%s\n\r" RESET,descr->status->state==EVENT_ARMED?"armed":"unarmed");
#ifdef EVNT_STATS
			printf("\tArmed: %8lu Triggered: %8lu  Disarmed: %8lu Error: %8lu\n\r",descr->status->stats.armed,descr->status->stats.triggered,descr->status->stats.disarmed,descr->status->stats.error);
#endif
			ret = 0;
		}
	}
	return(ret);
}

// Internal functions ----------------------------------------------------------
local volatile event_t* evntListRemoveHead(evntList_t *list)
{
	volatile event_t *event = NULL;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if(list != NULL && list->head != NULL)
		{
			event = list->head;
			list->head = event->next;
			if(list->tail == event)
				list->tail = NULL;
			list->size--;
		}
	}

	return event;
}

local int evntListAdd(evntList_t *list, volatile event_t *event)
{
	if(list == NULL || event == NULL)
		return -1;

	event->next = NULL;

	if(list->tail == NULL)
	{
		list->head = event;
		list->tail = event;
	}
	else
	{
		list->tail->next = event;
		list->tail = event;
	}

	list->size++;
	return 0;
}

local int evntListRemove(evntList_t *list, volatile event_t *event)
{
	// Check for null pointers
	if(list == NULL || event == NULL)
		return -1;

	// If the list is empty, nothing to remove
	if(list->head == NULL)
		return -1;

	// If the event to remove is the head of the list
	if(list->head == event)
	{
		list->head = event->next;
		if(list->tail == event)
			list->tail = NULL;
		list->size--;
		return 0;
	}

	// Search for the event in the list
	volatile event_t *prev = list->head;
	while(prev->next != NULL && prev->next != event)
	{
		prev = prev->next;
	}

	// If the event was not found
	if(prev->next == NULL)
		return -1;

	// Remove the event from the list
	prev->next = event->next;
	if(list->tail == event)
		list->tail = prev;
	list->size--;
	return 0;
}

// External functions ----------------------------------------------------------
// Get an event by name
volatile event_t* evntGetEvent(char *name)
{
	// Walk the table of events
	evntDescriptor_t *descr = (evntDescriptor_t *)&__start_EVNT_TABLE;
	for(; descr < (evntDescriptor_t *)&__stop_EVNT_TABLE; ++descr)
	{
		if(!strcmp(name,descr->name))
			return(descr->status);
	}

	return(NULL);
}

// Arm an event for a state machine
evntState_t evntArm(volatile event_t *event)
{
	evntState_t ret = EVENT_ARMED;
	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		event->state = EVENT_ARMED;

		// Remove the event from the disarmed list
		if(!evntListRemove(&evntListDisarmed, event))
		{
			// Add the event to the armed list
			evntListAdd(&evntListArmed, event);
		}
		// Else the event was not in the disarmed list, return an error
		else
		{
#ifdef EVNT_STATS
			event->stats.error++;
#endif
			ret = event->state = EVENT_ERROR;
		}
	} // End of critical section

#ifdef EVNT_STATS
	++event->stats.armed;
#endif

	return(ret);
}

// Arm a system-wide event (no waiting state machine).
// Used for self-arming events such as the system tick, whose handler runs
// directly out of evntDispatch and re-arms the event itself.
evntState_t evntArmSystem(volatile event_t *event)
{
	evntState_t ret = EVENT_ARMED;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		event->stateMachine = NULL;

		// Remove the event from the disarmed list and add to armed list.
		// Also allow re-arm when state is still EVENT_TRIGGERED: this covers the
		// narrow window between evntListRemoveHead and evntArmSystem in evntDispatch
		// where the ISR fires and evntTrigger returns the current state unchanged.
		if(!evntListRemove(&evntListDisarmed, event) || event->state == EVENT_TRIGGERED)
		{
			event->state = EVENT_ARMED;
			evntListAdd(&evntListArmed, event);
		}
		else
		{
#ifdef EVNT_STATS
			event->stats.error++;
#endif
			ret = event->state = EVENT_ERROR;
		}
	}

#ifdef EVNT_STATS
	if(ret == EVENT_ARMED)
		++event->stats.armed;
#endif

	return(ret);
}

// Disarm an event
evntState_t evntDisarm(volatile event_t *event)
{
	evntState_t ret = EVENT_DISARMED;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// stateMachine/state are shared with ISR context (evntTrigger), so
		// update them inside the critical section to avoid a torn write.
		event->stateMachine = NULL;
		event->state = EVENT_DISARMED;

		// Remove the event from the armed list
		if(!evntListRemove(&evntListArmed, event))
		{
			// Add the event to the disarmed list
			evntListAdd(&evntListDisarmed, event);
		}
		else
		{
#ifdef EVNT_STATS
			event->stats.error++;
#endif
			ret = EVENT_ERROR;
		}
	} // End of critical section

#ifdef EVNT_STATS
	++event->stats.disarmed;
#endif

	return(ret);
}

// Trigger an event
evntState_t evntTrigger(volatile event_t *event, int triggerType)
{
	evntState_t   ret;

	// Direct ISR-context handler (registered with ADD_EVENT_ISR): run it
	// immediately in the caller's (interrupt) context and do NOT queue the
	// event for evntDispatch(). The handler is the whole behaviour, so this
	// fires whether or not an FSM has armed the event and leaves the event's
	// list membership untouched (it stays on the disarmed list).
	if(event->descr->isrHandler != NULL)
	{
		event->triggerType = triggerType;
#ifdef EVNT_STATS
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			++event->stats.triggered;
		}
#endif
		event->descr->isrHandler(event);
		return(EVENT_TRIGGERED);
	}

	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// Remove the event from the armed list
		if(!evntListRemove(&evntListArmed, event))
		{
			ret = event->state = EVENT_TRIGGERED;
			event->triggerType = triggerType;
			// Add the event to the triggered list
			evntListAdd(&evntListTriggered, event);
#ifdef EVNT_STATS
			++event->stats.triggered;
#endif
		}
		// Else the event was not armed — nobody is waiting, this is normal
		// for queue events fired from ISR context when no SM has called evntWait.
		// Leave state unchanged and do not count as an error.
		else
		{
			ret = event->state;
		}
	} // End critical section of code

	return(ret);
}

// Wait for an event to be triggered
evntState_t evntWait(volatile event_t *event, 
					 int eventType,
					 fsmHandler_t fsmState)
{
	// fsmGetCurrentStateMachine() reads a main-loop-only global (currStateMachine)
	// that no ISR modifies, so resolve it before entering the critical section.
	volatile fsmStateMachine_t *stateMachine = fsmGetCurrentStateMachine();

	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		event->type = eventType;
		event->stateMachine = stateMachine;
		event->fsmState = fsmState;
		if(evntArm(event) == EVENT_ARMED)
		{
			// Put the state machine in the wait queue
			fsmWait(stateMachine);
		}
		else
		{
#ifdef EVNT_STATS
			event->stats.error++;
#endif
			event->state = EVENT_ERROR;
		}
	} // End of critical section

	return(event->state);
}

// Default event handler for events without a user defined handler
osStatus_t evntHandler(volatile event_t *event)
{
	osStatus_t ret = OS_ERROR;

	// If the event trigger type matches the current event type...
	if(event->type == event->triggerType)
	{
		if(event->stateMachine != NULL && event->fsmState != NULL)
		{
			fsmSetNextState(event->stateMachine, event->fsmState);
			// Move the state machine associated with the event to the ready queue
			fsmReady(event->stateMachine);
		}
		ret = OS_OK;
	}

	return(ret);
}

// Initialize the system events
int evntInit(void)
{
	evntDescriptor_t    *descr = (evntDescriptor_t *)&__start_EVNT_TABLE;
	int count = 0;

	// Walk the table of events
	for(; descr < (evntDescriptor_t *)&__stop_EVNT_TABLE; ++descr)
	{
		descr->status->state = EVENT_DISARMED;
		evntListAdd(&evntListDisarmed, descr->status);
		count++;
	}

	return(count);
}

// Scan the trigger event list and call the appropriate event handlers
int evntDispatch(void)
{
	int     ret = 0;
	volatile event_t *event;

	// Drain the triggered list. evntListRemoveHead atomically returns NULL when
	// the list is empty, so drive the loop off its return value rather than a
	// racy multi-byte read of evntListTriggered.size.
	for(;;)
	{
		// Atomically remove the head (protection is inside evntListRemoveHead).
		event = evntListRemoveHead(&evntListTriggered);

		// Call the event handler for the event
		if(event == NULL)
			break;
		if(event->descr->handler)
			event->descr->handler(event);

		// System events (no associated state machine) manage their own
		// list membership inside the handler (e.g. self-arming tick).
		// Only auto-disarm FSM-bound events here.
		if(event->stateMachine != NULL)
		{
			// evntListDisarmed is only accessed from main-loop context —
			// no ISR touches it, so no atomic protection is needed.
			evntListAdd(&evntListDisarmed, event);

			// evntListArmed IS shared with ISR context (evntTrigger removes
			// from it), so the scan and any removals must be atomic.
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
			{
				// Remove any other armed events belonging to the same state
				// machine — they are now stale since the SM is being woken.
				volatile event_t *armedEvent = evntListArmed.head;
				while(armedEvent != NULL)
				{
					volatile event_t *nextArmed = armedEvent->next;
					if(armedEvent->stateMachine == event->stateMachine)
					{
						evntListRemove(&evntListArmed, armedEvent);
						evntListAdd(&evntListDisarmed, armedEvent);
					}
					armedEvent = nextArmed;
				}
			} // End critical section — armed list scan
		}

		++ret;
	}
	return(ret);
}
