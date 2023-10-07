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
  FSM_DRV = 0b00000000,
  FSM_SYS = 0b01000000,
  FSM_SRV = 0b10000000,
  FSM_APP = 0b11000000
}fsmType_t;

// State machine priority
typedef uint8_t fsmPriority_t;

// Forward declare the STATE_MACHINE and STATE_DESCR structures
struct STATE_MACHINE_TYPE;
struct STATE_MACHINE_DESCR_TYPE;

// State handler function pointer type
typedef int (*fsmHandler_t)(volatile struct STATE_MACHINE_TYPE *state);

// State machine status. This data is stored in RAM and updated as the state machine progresses
// through it's various states. Each row in the state machine descriptor table has a pointer to 
// an instance of this data structure
typedef struct STATE_MACHINE_TYPE
{
  bool				                    initialCall;
  fsmHandler_t                          prevState, currState, nextState;
  volatile struct STATE_MACHINE_TYPE             *next;
  const struct STATE_MACHINE_DESCR_TYPE *stateMachineDescr;
} fsmStateMachine_t;

// Row in the state machine descriptor table. Provides the name of the state machine and pointers
// to the state machine status and the state machine table of function pointers
typedef struct STATE_MACHINE_DESCR_TYPE
{
  const char        *name;
  volatile fsmStateMachine_t *stateMachine;
  fsmHandler_t      initHandler;
  fsmPriority_t     priority;
  void              *instance;
} fsmStateMachineDescr_t;

// Adds a new state machine to the list of state machines handled by the FSM manager
#define ADD_STATE_MACHINE(stateMachineName, smInitHandler, smPriority, ...)	\
                          int smInitHandler(volatile fsmStateMachine_t *stateMachine); \
                          const static fsmStateMachineDescr_t SECTION(FSM_TABLE) CONCAT(stateMachineName,_descr); \
						  volatile fsmStateMachine_t stateMachineName = {.prevState = NULL, .currState = NULL, .nextState = smInitHandler, .stateMachineDescr = &CONCAT(stateMachineName,_descr) }; \
						  const static fsmStateMachineDescr_t SECTION(FSM_TABLE) CONCAT(stateMachineName,_descr) = { .name = #stateMachineName, .stateMachine = &stateMachineName, .initHandler = smInitHandler, .priority = smPriority, .instance = DEFAULT_OR_ARG(,##__VA_ARGS__,__VA_ARGS__,NULL)};

// Exported Functions ---------------------------------------------------------                                                          
void        fsmInit();
uint32_t	fsmScanCycle(void);
const char* fsmGetCurrentStateMachineName();
volatile fsmStateMachine_t* fsmGetCurrentStateMachine();
void*       fsmGetInstance(volatile fsmStateMachine_t *stateMachine);
bool		fsmInitialCall();
fsmHandler_t fsmCurrentState(volatile fsmStateMachine_t *stateMachine);
fsmHandler_t fsmPreviousState(volatile fsmStateMachine_t *stateMachine);
int			fsmNextState(volatile fsmStateMachine_t *stateMachine, fsmHandler_t handler);
int         fsmReady(volatile fsmStateMachine_t *stateMachine);
int         fsmWait(volatile fsmStateMachine_t *stateMachine);
int			fsmStop(volatile fsmStateMachine_t *stateMachine);
void		fsmDispatch(void);

#endif /* __FSM_H */
