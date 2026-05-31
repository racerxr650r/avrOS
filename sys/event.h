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
/**
 * @brief Lifecycle state of an event object.
 *
 * Transitions:
 *   DISARMED → ARMED (evntArm / evntArmSystem / evntWait) →
 *   TRIGGERED (evntTrigger) → DISARMED (evntDispatch auto-disarm)
 *
 * ERROR is set when an operation is called on an event that is not in the
 * expected list (e.g. evntArm called on an event not on the disarmed list).
 */
typedef enum
{
	EVENT_ERROR = -1,   /**< Operation failed; event left in inconsistent state. */
	EVENT_IDLE = 0,     /**< Alias for DISARMED; initial power-on state. */
	EVENT_ARMED,        /**< Event is waiting to be triggered by a producer. */
	EVENT_DISARMED,     /**< Event is inactive; not waiting for a trigger. */
	EVENT_TRIGGERED     /**< Producer has fired; pending dispatch to handler. */
} evntState_t;

struct EVENT_DESCR_TYPE;
struct EVENT_TYPE;

/**
 * @brief Per-event diagnostic counters (compiled in with EVNT_STATS).
 */
typedef struct EVENT_STATS
{
	uint32_t	armed;     /**< Number of times the event was successfully armed. */
	uint32_t	triggered; /**< Number of times the event was successfully triggered. */
	uint32_t	disarmed;  /**< Number of times the event was explicitly disarmed. */
	uint32_t	error;     /**< Number of times an operation found the event in an unexpected state. */
}evntStats_t;

/**
 * @brief Event runtime status (RAM).
 *
 * The sub-type contract is implemented by two fields:
 *
 *   - @c type        — set by evntWait(); the sub-condition the consumer is
 *                      waiting for.
 *   - @c triggerType — set by evntTrigger(); the sub-condition the producer
 *                      is signaling.
 *
 * evntHandler() releases the waiting state machine only when
 * @c type == @c triggerType, allowing one event object to multiplex several
 * sub-conditions without spurious wakeups.
 *
 * @note Sub-type 0 is reserved as the "armed but not yet triggered" sentinel;
 *       new modules should number sub-types starting at 1.
 *       See doc/SDD.md §4.3.4 for full conventions.
 */
typedef struct EVENT_TYPE
{
	evntState_t						state;          /**< Current lifecycle state of this event. */
	volatile fsmStateMachine_t 		*stateMachine;  /**< State machine waiting on the event, or NULL for system events. */
	volatile fsmHandler_t			fsmState;		/**< FSM state handler to resume when the event fires. */
	uint8_t							type;			/**< Sub-type the consumer is waiting for (set by evntWait). */
	uint8_t							triggerType;	/**< Sub-type the producer signaled (set by evntTrigger). */
	volatile struct EVENT_TYPE 		*next;          /**< Intrusive linked-list pointer for armed/triggered/disarmed lists. */
	const struct EVENT_DESCR_TYPE 	*descr;         /**< Pointer to the flash-resident event descriptor. */
#ifdef EVNT_STATS
	evntStats_t       				stats;          /**< Diagnostic counters (requires EVNT_STATS). */
#endif
}event_t;

/**
 * @brief Event handler callback type.
 *
 * Called by evntDispatch() in main-loop context when an event transitions
 * to TRIGGERED.  The default implementation is evntHandler().
 *
 * @param event  Pointer to the triggered event.
 * @return 0 on success, negative on error.
 */
typedef int (*evntHandler_t)(volatile event_t *event);

/**
 * @brief Flash-resident event descriptor (one per ADD_EVENT instance).
 *
 * Placed in the EVNT_TABLE linker section; walked at init time by evntInit()
 * and at runtime by the CLI command.
 */
typedef struct EVENT_DESCR_TYPE
{
	char    		*name;    /**< Human-readable name used by the CLI. */
	volatile event_t	*status;  /**< Pointer to the RAM-resident event_t. */
	evntHandler_t	handler;  /**< Callback invoked by evntDispatch() on trigger. */
}evntDescriptor_t;

/**
 * @brief Intrusive singly-linked list of event objects.
 *
 * Three instances exist at runtime: evntListArmed, evntListDisarmed,
 * evntListTriggered.  All mutations must be performed inside
 * ATOMIC_BLOCK(ATOMIC_RESTORESTATE).
 */
typedef struct EVENT_LIST
{
    volatile event_t *head; /**< First element, or NULL if empty. */
    volatile event_t *tail; /**< Last element, or NULL if empty. */
    uint32_t size;          /**< Number of elements currently in the list. */
} evntList_t;

// External Functions ----------------------------------------------------------
/**
 * @brief Return the current lifecycle state of an event.
 *
 * @param event  Pointer to the event.
 * @return Current @ref evntState_t value.
 */
static inline evntState_t evntGetStatus(volatile event_t *event)
{
	return(event->state);
}

/**
 * @brief Return the state machine currently running (i.e. the caller).
 *
 * Convenience wrapper around fsmGetCurrentStateMachine().
 *
 * @return Pointer to the currently executing fsmStateMachine_t.
 */
static inline volatile fsmStateMachine_t* evntGetStateMachine()
{
	return(fsmGetCurrentStateMachine());
}

/**
 * @brief Return the sub-type the consumer registered with evntWait().
 *
 * @param event  Pointer to the event.
 * @return The @c type field value.
 */
static inline int evntGetType(volatile event_t *event)
{
	return(event->type);
}

/**
 * @brief Return the sub-type the producer supplied to evntTrigger().
 *
 * @param event  Pointer to the event.
 * @return The @c triggerType field value.
 */
static inline int evntGetTrigger(volatile event_t *event)
{
	return(event->triggerType);
}

/**
 * @brief Look up an event by name.
 *
 * Walks the EVNT_TABLE linker section.
 *
 * @param name  Null-terminated name string to search for.
 * @return Pointer to the matching event_t, or NULL if not found.
 */
volatile event_t* evntGetEvent(char *name);

/**
 * @brief Arm an event for the state machine stored in event->stateMachine.
 *
 * Moves the event from the disarmed list to the armed list.  The caller
 * must set @c event->stateMachine before calling.  Does NOT call fsmWait();
 * use evntWait() for the combined arm-and-suspend operation.
 *
 * @param event  Pointer to the event to arm.
 * @return EVENT_ARMED on success, EVENT_ERROR if the event was not on the
 *         disarmed list.
 */
evntState_t evntArm(volatile event_t *event);

/**
 * @brief Arm a system-wide event that has no associated state machine.
 *
 * Used by self-arming events (e.g. the system tick) whose handler calls
 * this function to re-arm the event after processing.  Allows re-arm
 * from EVENT_TRIGGERED state to handle the case where the ISR fires
 * while the event is being dispatched.
 *
 * @param event  Pointer to the system event to arm.
 * @return EVENT_ARMED on success, EVENT_ERROR on failure.
 */
evntState_t evntArmSystem(volatile event_t *event);

/**
 * @brief Explicitly disarm an event, cancelling any pending wait.
 *
 * Moves the event from the armed list back to the disarmed list.
 *
 * @param event  Pointer to the event to disarm.
 * @return EVENT_DISARMED on success, EVENT_ERROR if the event was not armed.
 */
evntState_t evntDisarm(volatile event_t *event);

/**
 * @brief Signal an event from a producer (ISR-safe).
 *
 * If the event is on the armed list it is moved to the triggered list and
 * @p subType is stored in @c event->triggerType.  If the event is not armed
 * (nobody is waiting) the call is a silent no-op — this is normal for queue
 * events fired from ISR context.
 *
 * @param event    Pointer to the event to trigger.
 * @param subType  Producer-defined sub-condition value.
 * @return EVENT_TRIGGERED on success, or the current event state if not armed.
 */
evntState_t evntTrigger(volatile event_t *event, int subType);

/**
 * @brief Suspend the current FSM until the specified event sub-type fires.
 *
 * Arms the event for the calling state machine, stores @p eventType as the
 * expected sub-type, caches @p fsmState as the handler to resume, and calls
 * fsmWait() to remove the SM from the ready queue.
 *
 * @param event      Pointer to the event to wait on.
 * @param eventType  Sub-type value that must match evntTrigger's subType to
 *                   wake this state machine.
 * @param fsmState   FSM state handler to call when the event fires.
 * @return The resulting event state (EVENT_ARMED or EVENT_ERROR).
 */
evntState_t evntWait(volatile event_t *event, int eventType, fsmHandler_t fsmState);

/**
 * @brief Default event handler — wakes the waiting FSM on sub-type match.
 *
 * Compares @c event->type with @c event->triggerType; if they match, calls
 * fsmSetNextState() and fsmReady() to resume the waiting state machine.
 * Assign a custom handler via ADD_EVENT to override this behaviour.
 *
 * @param event  Pointer to the triggered event.
 * @return 0 if the state machine was woken, -1 if the sub-types did not match.
 */
int evntHandler(volatile event_t *event);

/**
 * @brief Initialize the event manager.
 *
 * Walks the EVNT_TABLE linker section and places every event on the
 * disarmed list.  Called once from sysInit().
 *
 * @return Number of events registered.
 */
int evntInit(void);

/**
 * @brief Dispatch all pending triggered events.
 *
 * Drains the triggered list, calling each event's handler and moving
 * FSM-bound events back to the disarmed list afterward.  System events
 * (stateMachine == NULL) manage their own list membership inside their
 * handler.  Called from fsmDispatch() on every scheduler cycle.
 *
 * @return Number of events dispatched.
 */
int evntDispatch(void);

// Macros ----------------------------------------------------------------------
/**
 * @brief Declare, register, and initialize an event object.
 *
 * Creates a flash-resident @ref evntDescriptor_t in the EVNT_TABLE linker
 * section and a RAM-resident @ref event_t initialized to EVENT_DISARMED.
 * evntInit() picks up all EVNT_TABLE entries at startup.
 *
 * Usage:
 * @code
 *   ADD_EVENT(myEvent);                   // uses default evntHandler
 *   ADD_EVENT(myEvent, myCustomHandler);  // custom handler
 * @endcode
 *
 * @param evntName  Token used as the C variable name for the event_t.
 * @param ...       Optional custom @ref evntHandler_t; defaults to evntHandler().
 */
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
