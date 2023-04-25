/*
 * fsm.h
 *
 * Header for the finite state machine manager. Includes macros to create new
 * state machines (ADD_STATE_MACHINE) and new states (ADD_STATE) for existing state
 * machines.
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

// Includes -------------------------------------------------------------------
#include "../avrOS.h"
#include "stdlib.h"

// Types ----------------------------------------------------------------------
// Type modifier for priority
typedef enum FSM_TYPE
{
  FSM_SYS = 0x00,
  FSM_DRV = 0b01000000,
  FSM_SRV = 0b10000000,
  FSM_APP = 0b11000000
}fsmType_t;

// State machine priority
typedef uint8_t fsmPriority_t;

struct STATE_MACHINE_TYPE;
struct STATE_MACHINE_DESCR_TYPE;

// Row in the state machine table. Provides the state name and the associated function pointer
// This data structure is stored in a named section in NVM. There is a unique named section 
// for each of the state machines created by the user
typedef struct
{
  const struct STATE_MACHINE_TYPE *stateMachine;
  const char	    *name;
  int			        (*funcPtr)(struct STATE_MACHINE_TYPE *state);
  const char*	    (*stateNameFuncPtr)();
} fsmState_t;

// State machine status. This data is stored in RAM and updated as the state machine progresses
// through it's various states. Each row in the state machine descriptor table has a pointer to 
// an instance of this data structure
typedef struct STATE_MACHINE_TYPE
{
  bool				initialCall;
  fsmState_t	*prevState, *currState, *nextState;
  const struct STATE_MACHINE_DESCR_TYPE *stateMachineDescr;
} fsmStateMachine_t;

// Row in the state machine descriptor table. Provides the name of the state machine and pointers
// to the state machine status and the state machine table of function pointers
typedef struct STATE_MACHINE_DESCR_TYPE
{
  const char        *name;
  fsmStateMachine_t *stateMachine;
  fsmPriority_t     priority;
  void              *instance;
} fsmStateMachineDescr_t;

// Adds a new state machine to the list of state machines handled by the FSM manager
#define ADD_STATE_MACHINE(stateMachineName, initStateName, smPriority, ...)	const static fsmStateMachineDescr_t CONCAT(stateMachineName,_descr); \
															const static fsmState_t				initStateName; \
															fsmStateMachine_t stateMachineName = {.prevState = NULL, .currState = NULL, .nextState = (fsmState_t *)&initStateName, .stateMachineDescr = &CONCAT(stateMachineName,_descr) }; \
															const static fsmStateMachineDescr_t SECTION(FSM_TABLES) CONCAT(stateMachineName,_descr) = { .name = #stateMachineName, .stateMachine = &stateMachineName, .priority = smPriority, .instance = DEFAULT_OR_ARG(,##__VA_ARGS__,__VA_ARGS__,NULL)};
// Adds a new state to an existing state machine handled by the FSM manager
#define ADD_STATE(stateMachineName, stateName, ...)	int CONCAT(stateName,Func)(fsmStateMachine_t *state); \
																const static fsmState_t SECTION(FSM_STATES) stateName = { .stateMachine = &stateMachineName, .name = #stateName, .funcPtr = &CONCAT(stateName,Func), .stateNameFuncPtr = DEFAULT_OR_ARG(,##__VA_ARGS__,__VA_ARGS__,NULL)};

// Exported Functions ---------------------------------------------------------                                                          
uint32_t	fsmScanCycle(void);
const char* fsmCurrentStateName();
const char* fsmCurrentStateMachineName();
bool		fsmInitialCall();
fsmState_t*	fsmCurrentState(fsmStateMachine_t *stateMachine);
fsmState_t* fsmPreviousState(fsmStateMachine_t *stateMachine);
int			fsmNextState(fsmStateMachine_t *stateMachine, const fsmState_t *state);
int			fsmStop(fsmStateMachine_t *stateMachine);
void		fsmDispatch(void);

#endif /* __FSM_H */
