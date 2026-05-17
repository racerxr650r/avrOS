/**
 * @file fsm.h
 *
 * Header for the finite state machine manager. Includes macros to create new
 * state machines (ADD_STATE_MACHINE) and initialization functions
 * (ADD_INITIALIZER)
 *
 * Created: 2/28/2021 4:14:29 PM
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
#ifndef __FSM_H
#define __FSM_H

/** @addtogroup fsm_manager
 * @{
 */

// Includes -------------------------------------------------------------------
#include "../avrOS.h"
#include "stdlib.h"

// Types ----------------------------------------------------------------------
/**----------------------------------------------------------------------------
 * @brief State machine priority class (top 2 bits of fsmPriority_t).
 *
 * The byte passed as the `priority` arg to ADD_STATE_MACHINE encodes both a
 * class (top 2 bits, this enum) and a 6-bit sub-priority. Lower numeric
 * values run first in the ready list. Within a class, sub-priority 0 runs
 * before sub-priority 63.
 *
 * Pick the class that matches the directory the module lives in:
 *   drv/ -> FSM_DRV, srv/ -> FSM_SRV, app/ -> FSM_APP. FSM_SYS is reserved.
 *
 * Conventions for sub-priority:
 *   0..15  - critical / latency-sensitive (drain UART, button debounce)
 *   16..47 - normal background work
 *   48..63 - catch-all / lowest within class (the CLI uses FSM_SRV | 0x3f)
 *
 * See doc/SDD.md sec. 4.2.3 for full guidance.
 */
typedef enum FSM_TYPE
{
	FSM_DRV = 0b00000000,	///< Driver priority (highest)
	FSM_SYS = 0b01000000,	///< System priority (reserved)
	FSM_SRV = 0b10000000,	///< Service priority
	FSM_APP = 0b11000000	///< Application priority (lowest)
}fsmType_t;

/**----------------------------------------------------------------------------
 * State machine priority type
 */
typedef uint8_t fsmPriority_t;

// Forward declare the STATE_MACHINE and STATE_DESCR structures
struct STATE_MACHINE_TYPE;
struct STATE_MACHINE_DESCR_TYPE;

/**----------------------------------------------------------------------------
 * State handler function pointer type
 */
typedef int (*fsmHandler_t)(volatile struct STATE_MACHINE_TYPE *state);

/**----------------------------------------------------------------------------
 * Initialization handler function pointer type
 */
typedef int (*initHandler_t)(const struct STATE_MACHINE_DESCR_TYPE *state);

/**----------------------------------------------------------------------------
 * State machine status type
 * 
 * This data is stored in RAM and updated as the state machine progresses
 * through it's various states. Each row in the state machine descriptor table
 * has a pointer to an instance of this data structure which is stored in RAM
 */
typedef struct STATE_MACHINE_TYPE
{
	const char								*currStateName;
	const char								*prevStateName;
	const char								*nextStateName;
	bool									initialCall;		///< Initial call to current state status
	fsmHandler_t							prevState;			///< Previous state function pointer
	fsmHandler_t							currState;			///< Current state function pointer
	fsmHandler_t							nextState;			///< Next state function pointer
	uint32_t								ticks;				///< Tick count for system tick wait
	volatile struct STATE_MACHINE_TYPE    	*next;				///< Next pointer used for the SM queues
	const struct STATE_MACHINE_DESCR_TYPE 	*stateMachineDescr;	///< Pointer to the state machine descriptor
} fsmStateMachine_t;

/**
 * State machine descriptor type
 * 
 * Row in the state machine descriptor table. Provides the name of the state 
 * machine and pointers to the state machine status. This data is store in
 * non-volatile memory
*/
typedef struct STATE_MACHINE_DESCR_TYPE
{
	const char					*name;			///< Name of the state machine
	volatile fsmStateMachine_t	*stateMachine;	///< Pointer to the state machine status
	union
	{
		fsmHandler_t 			fsmHandler;
		initHandler_t			initHandler;
	}handler;
	fsmPriority_t				priority;		///< Priority of the state machine
	void						*instance;		///< Pointer to additional data required by the state machine
} fsmStateMachineDescr_t;

// Finite State Machine Macros ------------------------------------------------
/**
 * @brief Add state machine to the application
 *
 * Adds a new state machine to the list of state machines handled by the FSM manager
 *
 * @param stateMachineName	Name of the state machine to add to the application, type char*
 * @param smInitHandler		Function pointer to the initial state for the state machine, type fsmHandler_t
 * @param smPriority		Priority of the state machine, type fsmPriority_t
 * @param ...				(Optional) pointer to additional data needed by the state machine, type void*
 */
#define ADD_STATE_MACHINE(stateMachineName, smInitHandler, smPriority, ...)	\
		int smInitHandler(volatile fsmStateMachine_t *stateMachine); \
		const static fsmStateMachineDescr_t SECTION(FSM_TABLE) CONCAT(stateMachineName,_descr); \
		volatile fsmStateMachine_t stateMachineName = {.currStateName = NULL, .prevStateName = NULL, .nextStateName = NULL, .initialCall = false, .prevState = NULL, .currState = NULL, .nextState = smInitHandler, .ticks = 0, .next = NULL, .stateMachineDescr = &CONCAT(stateMachineName,_descr)}; \
		const static fsmStateMachineDescr_t SECTION(FSM_TABLE) CONCAT(stateMachineName,_descr) = { .name = #stateMachineName, .stateMachine = &stateMachineName, .handler.fsmHandler = smInitHandler, .priority = smPriority, .instance = DEFAULT_OR_ARG(,##__VA_ARGS__,__VA_ARGS__,NULL)};
/**
 * @brief Add an initializer to the application
 *
 * Adds a new initializer called by the FSM manager at startup
 *
 * @param stateMachineName	Name of the initializer to add to the application, type char*
 * @param smHandler			Function pointer to the initializer handler, type initHandler_t
 * @param ...				(Optional) pointer to additional data needed by the initializer, type void*
 */
#define ADD_INITIALIZER(stateMachineName, smHandler, ...)	\
		int smHandler(const fsmStateMachineDescr_t *stateMachineDescr); \
		const static fsmStateMachineDescr_t SECTION(FSM_TABLE) CONCAT(stateMachineName,_descr) = { .name = #stateMachineName, .stateMachine = NULL, .handler.initHandler = smHandler, .priority = 0, .instance = DEFAULT_OR_ARG(,##__VA_ARGS__,__VA_ARGS__,NULL)};
/**
 * @brief Set the next state of the given state machine
 *
 * Wrapper macro that calls fsmSetNextStateVerbose() with the stringified state name.
 *
 * @param stateMachine	Pointer to state machine
 * @param stateName	Function implementing the next state
 */
#define fsmSetNextState(stateMachine, stateName) \
		fsmSetNextStateVerbose(stateMachine, stateName, #stateName)

// !FSM_STATS
/*#else
#define ADD_STATE_MACHINE(stateMachineName, smInitHandler, smPriority, ...)	\
		int smInitHandler(volatile fsmStateMachine_t *stateMachine); \
		const static fsmStateMachineDescr_t SECTION(FSM_TABLE) CONCAT(stateMachineName,_descr); \
		volatile fsmStateMachine_t stateMachineName = {.currStateName = NULL, .prevStateName = NULL, .nextStateName = NULL, .initialCall = false, .prevState = NULL, .currState = NULL, .nextState = smInitHandler, .ticks = 0, .next = NULL, .stateMachineDescr = &CONCAT(stateMachineName,_descr)}; \
		const static fsmStateMachineDescr_t SECTION(FSM_TABLE) CONCAT(stateMachineName,_descr) = { .name = #stateMachineName, .stateMachine = &stateMachineName, .handler.fsmHandler = smInitHandler, .priority = smPriority, .instance = DEFAULT_OR_ARG(,##__VA_ARGS__,__VA_ARGS__,NULL)};

#define ADD_INITIALIZER(stateMachineName, smHandler, ...)	\
		int smHandler(const fsmStateMachineDescr_t *stateMachineDescr); \
		const static fsmStateMachineDescr_t SECTION(FSM_TABLE) CONCAT(stateMachineName,_descr) = { .name = #stateMachineName, .stateMachine = NULL, .handler.initHandler = smHandler, .priority = 0, .instance = DEFAULT_OR_ARG(,##__VA_ARGS__,__VA_ARGS__,NULL)};
// Define fsmSetNextState to use fsmSetNextStateBasic
#define fsmSetNextState(stateMachine, stateName) \
		fsmSetNextStateBasic(stateMachine, stateName)
#endif*/

// Exported Functions --------------------------------------------------------
/**
 * @brief Get the scan cycle count
 *
 * Returns the number of times that fsmDispatch() has been called in the
 * application main loop. On the initial call to fsmDispatch(), this value will
 * be 1.
 *
 * @return The current scan cycle count
 */
uint32_t fsmScanCycle();
/**
 * @brief Get the current state machine name
 *
 * Returns a pointer to the char* name of the current state machine that is
 * being executed by the finite state machine manager.
 *
 * @return Pointer to the name string of the current state machine
 */
const char* fsmGetCurrentStateMachineName();
/**
 * @brief Get the current state machine
 *
 * Returns a pointer to the current state machine that is being executed by the
 * finite state machine manager.
 *
 * @return Pointer to the current state machine
 */
volatile fsmStateMachine_t* fsmGetCurrentStateMachine();
/**
 * @brief Get the state machine instance
 *
 * Returns a pointer to the state machine instance. The instance is state
 * machine specific information that is provided when the state machine is
 * added using the ADD_STATE_MACHINE() macro.
 *
 * @param stateMachine	Pointer to the state machine
 * @return Pointer to the state machine instance data
 */
void* fsmGetInstance(volatile fsmStateMachine_t *stateMachine);
/**
 * @brief Get the initializer instance
 *
 * Returns a pointer to the initializer instance. The instance is initializer
 * specific information that is provided when the initializer is
 * added using the ADD_INITIALIZER() macro.
 *
 * @param stateMachineDescr	Pointer to the state machine descriptor
 * @return Pointer to the instance data
 */
void* initGetInstance(const fsmStateMachineDescr_t *stateMachineDescr);
/**
 * @brief Initial call to the current state?
 *
 * Returns true if this is the first call to the current state since the
 * previous state change.
 *
 * @return true if this is the first call to the current state, false otherwise
 */
bool fsmIsInitialCall();
/**
 * @brief Get pointer to the current state machine state (function)
 *
 * Returns a function pointer that implements the given state machine's current
 * state.
 *
 * @param stateMachine	[in] Pointer to state machine
 * @return Function pointer to the current state handler
 */
fsmHandler_t fsmGetCurrentState(volatile fsmStateMachine_t *stateMachine);
/**
 * @brief Get the name of the current state machine state (function)
 *
 * Returns a pointer to the string name of the given state machine's current
 * state.
 *
 * @param stateMachine	[in] Pointer to state machine
 * @return Pointer to the current state name string
 */
const char* fsmGetCurrentStateName(volatile fsmStateMachine_t *stateMachine);
/**
 * @brief Get the name of the previous state machine state (function)
 *
 * Returns a pointer to the string name of the given state machine's previous
 * state.
 *
 * @param stateMachine	[in] Pointer to state machine
 * @return Pointer to the previous state name string
 */
const char* fsmGetPreviousStateName(volatile fsmStateMachine_t *stateMachine);
/**
 * @brief Get pointer to the previous state
 *
 * Returns function pointer to the previous state prior to the latest state change.
 *
 * @param stateMachine	[in] Pointer to state machine
 * @return Function pointer to the previous state handler
 */
fsmHandler_t fsmGetPreviousState(volatile fsmStateMachine_t *stateMachine);
/**
 * @brief Set the next state of the given state machine
 *
 * Sets the next state of the state machine. The next time this state machine
 * is called by the state machine manager it will call the function provided by
 * the handler parameter.
 *
 * @param stateMachine	[in] Pointer to state machine
 * @param handler		[in] Pointer to function implementing the next state
 * @return 0 on success, non-zero on failure
 */
int fsmSetNextStateBasic(volatile fsmStateMachine_t *stateMachine,
						fsmHandler_t handler);
/**
 * @brief Set the next state of the given state machine (verbose)
 *
 * Sets the next state of the state machine. The next time this state machine
 * is called by the state machine manager it will call the function provided by
 * the handler parameter.
 *
 * @param stateMachine	[in] Pointer to state machine
 * @param handler		[in] Pointer to function implementing the next state
 * @param name			[in] String containing the name of the next state
 * @return 0 on success, non-zero on failure
 */
int fsmSetNextStateVerbose(volatile fsmStateMachine_t *stateMachine,
							fsmHandler_t handler,
							const char *name);
/**
 * @brief Initialize the Finite State Manager
 *
 * This function is called by sysInit() during the initialization phase. It
 * walks the const table of state machines and builds the ready queue for first
 * call of fsmDispatch() in the application main loop.
 */
void fsmInit();
/**
 * @brief Move the given state machine to the ready queue
 *
 * Moves the provided state machine to the ready queue that is maintained by
 * the finite state machine manager. The ready queue contains the state
 * machines that are ready to be scheduled (called).
 *
 * @param stateMachine	[in] Pointer to state machine
 * @return 0 on success, non-zero on failure
 */
int	fsmReady(volatile fsmStateMachine_t *stateMachine);
/**
 * @brief Move the given state machine to the wait queue
 *
 * Moves the provided state machine to the wait queue that is maintained by the
 * finite state machine manager. The wait queue contains state machines that
 * are waiting on a timer or event.
 *
 * @param stateMachine	[in] Pointer to state machine
 * @return 0 on success, non-zero on failure
 */
int	fsmWait(volatile fsmStateMachine_t *stateMachine);
/**
 * @brief Move the given state machine to the stopped queue
 *
 * Moves the provided state machine to the stopped queue that is maintained by
 * the finite state machine manager. The stopped queue contains state machines
 * that have stopped execution. These state machines will not run again, unless
 * fsmReady() is called for that state machine.
 *
 * @param stateMachine	[in] Pointer to state machine
 * @return 0 on success, non-zero on failure
 */
int	fsmStop(volatile fsmStateMachine_t *stateMachine);
/**
 * @brief Update the state machines in the wait queue waiting on the system timer
 */
void fsmUpdateWaitTicks();
/**
 * @brief Set the state machine to sit in the wait queue for x ticks
 *
 * @param stateMachine	[in] Pointer to state machine
 * @param ticks			[in] Number of ticks to wait
 */
void fsmWaitTicks(volatile fsmStateMachine_t*stateMachine, uint32_t ticks);
/**
 * @brief Set the state machine to sit in the wait queue for x milliseconds
 *
 * @param stateMachine	[in] Pointer to state machine
 * @param ms			[in] Number of milliseconds to wait
 */
void fsmWaitMilliseconds(volatile fsmStateMachine_t*stateMachine, uint16_t ms);
/**
 * @brief Execute the state machines in the ready queue
 *
 * This function implements the state machine manager. It is called in the
 * application main loop. This function calls the state machines in the ready
 * queue in priority order. Once it has called all of the state machines in the
 * ready queue, it will cycle back to the head of the ready queue and start
 * again. It will return once there are no state machines in the ready queue.
 */
void	fsmDispatch(void);

/** @} */ // end of fsm_manager

#endif /* __FSM_H */
