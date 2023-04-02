/*
 * fsm.c
 *
 * Implements the finite state machine manager and API
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
// Includes -------------------------------------------------------------------
#include "../avrOS.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Externs --------------------------------------------------------------------
extern void *__start_FSM_TABLES,*__stop_FSM_TABLES,*__start_FSM_STATES,*__stop_FSM_STATES;

// Internal Globals -----------------------------------------------------------
static uint32_t scanCycle = 0;
static fsmStateMachineDescr_t	*currStateMachine = NULL;
static char initString[] = {"Init"};

// Internal functions ---------------------------------------------------------
//static fsmStateMachineDescr_t*	fsmGetStateMachineDescr(const char *name);
static fsmStateMachine_t*		fsmGetStateMachine(const char *name);
static fsmState_t*				fsmGetState(fsmStateMachine_t *stateMachine, const char *name);

// CLI Commands ---------------------------------------------------------------
#ifdef FSM_CLI
ADD_COMMAND("fsm",fsmCmd,true);
#endif // FSM_CLI

int fsmCmd(int argc, char *argv[])
{
	// The UNUSED macro prevents the compiler from warning about unused variables
	// Include these only if the function does not use the argc and argv parameters
	UNUSED(argc);
	UNUSED(argv);

	// Walk the table of state machines
	fsmStateMachineDescr_t *descr = (fsmStateMachineDescr_t *)&__start_FSM_TABLES;
	for(; descr < (fsmStateMachineDescr_t *)&__stop_FSM_TABLES; ++descr)
	{
		// If this State Machine is marked visible...
		if(descr->visible == true)
		{
			// If no State Machine given, or the specified State Machine is this one...
			if(argc<2 || (argc==2 && !strcmp(descr->name,argv[1])))
			{
				// Display the State Machine Name
				printf("State Machine: %s\n\r",descr->name);

				// Walk the state table for the current state machine
				fsmState_t *state = (fsmState_t *)&__stop_FSM_STATES;
				for(; state >= (fsmState_t *)&__start_FSM_STATES; state--)
				{
					// If this State is a member of the current State Machine...
					if(state->stateMachine->stateMachineDescr == descr)
						printf("\t%c%s\n\r",(descr->stateMachine->currState == state)?'>':' ',state->name);
				}
			}
		}
	}

	return(0);
}

#ifdef FSM_CLI
ADD_COMMAND("fsmStop",fsmStopCmd);
#endif // FSM_CLI

int fsmStopCmd(int argc, char *argv[])
{
	// If command line includes a name of a state machine...
	if((argc == 2) && (argv[1] != NULL))
	{
		fsmStateMachine_t *stateMachine = fsmGetStateMachine(argv[1]);
		
		if(stateMachine!=NULL)
			return(fsmStop(stateMachine));
	}
	return(-1);
}

#ifdef FSM_CLI
ADD_COMMAND("fsmStart",fsmStartCmd);
#endif // FSM_CLI

int fsmStartCmd(int argc, char *argv[])
{
  // If command line includes a name of a state machine...
  if((argc == 3) && (argv[1] != NULL) && (argv[2] != NULL))
  {
    fsmStateMachine_t	*stateMachine = fsmGetStateMachine(argv[1]);
	fsmState_t			*state = fsmGetState(stateMachine,argv[2]);
	
    if(stateMachine->currState == NULL && stateMachine != NULL && state != NULL)
      return(fsmNextState(stateMachine,state));
  }
  return(-1);
}

// Internal Functions ---------------------------------------------------------
/*static fsmStateMachineDescr_t *fsmGetStateMachineDescr(const char *name)
{
	// Walk the table of state machines
	fsmStateMachineDescr_t *descr = (fsmStateMachineDescr_t *)&__start_FSM_TABLES;
	
	for(; descr < (fsmStateMachineDescr_t *)&__stop_FSM_TABLES; ++descr)
		if(strcmp(name,descr->name) == 0)
			return(descr);

	return(NULL);
}*/

static fsmStateMachine_t *fsmGetStateMachine(const char *name)
{
  // Walk the table of state machines
  fsmStateMachineDescr_t *descr = (fsmStateMachineDescr_t *)&__start_FSM_TABLES;
  
  for(; descr < (fsmStateMachineDescr_t *)&__stop_FSM_TABLES; ++descr)
    if(strcmp(name,descr->name) == 0)
      return(descr->stateMachine);

  return(NULL);
}

static fsmState_t *fsmGetState(fsmStateMachine_t *stateMachine, const char *name)
{
  fsmState_t *state = (fsmState_t *)&__start_FSM_STATES;
  for(; state < (fsmState_t *)&__stop_FSM_STATES; state++)
    if((strcmp(name,state->name) == 0) && (state->stateMachine == stateMachine))
      return(state);
	  
  return(NULL);
}

// External Functions ---------------------------------------------------------
// Get the scan cycle count
uint32_t fsmGetScanCycle()
{
	return(scanCycle);
}

// Get the current state name
const char* fsmCurrentStateName()
{
	const char *ret = NULL;
	
	if(currStateMachine!=NULL)
	{
		if(currStateMachine->stateMachine->currState->stateNameFuncPtr != NULL)
			ret = currStateMachine->stateMachine->currState->stateNameFuncPtr();
		else
			ret = currStateMachine->stateMachine->currState->name;
	}
	else
		ret = initString;

	return(ret);
}

// Get the current state machine name
const char* fsmCurrentStateMachineName()
{
	const char *ret = NULL;
	
	if(currStateMachine!=NULL)
		ret = currStateMachine->name;
	else
		ret = initString;
	
	return(ret);
}

// Is this the first call of the state function after a state change?
bool fsmInitialCall()
{
	return(currStateMachine->stateMachine->initialCall);
}

// Return pointer to the current state
fsmState_t* fsmCurrentState(fsmStateMachine_t *stateMachine)
{
	return(stateMachine->currState);
}

// Return pointer to the previous state
fsmState_t* fsmPreviousState(fsmStateMachine_t *stateMachine)
{
	return(stateMachine->prevState);
}

// Change the given state machine to the given state
int fsmNextState(fsmStateMachine_t *stateMachine, const fsmState_t *state)
{
  if(stateMachine)
    stateMachine->nextState = (fsmState_t *)state;
  else
    return(-1);

  return(0);
}

// Stop the given state machine
int fsmStop(fsmStateMachine_t *stateMachine)
{
	if(stateMachine)
	{
		stateMachine->prevState = stateMachine->currState;
		stateMachine->nextState = stateMachine->currState = NULL;
		INFO("State Machine %s Stopped",stateMachine->stateMachineDescr->name);
	}
	else
		return(-1);

	return(0);
}

// Finite State Machine Dispatcher. This function steps through the state machine table and calls the current state function for each
void fsmDispatch(void)
{
	++scanCycle;
	// Walk the table of state machines
	currStateMachine = (fsmStateMachineDescr_t *)&__start_FSM_TABLES;
	for(; currStateMachine < (fsmStateMachineDescr_t *)&__stop_FSM_TABLES; ++currStateMachine)
	{
		fsmStateMachine_t	*state = currStateMachine->stateMachine;
	  
		// If a next state has been set, update the current state
		if(state->nextState != NULL)
		{
			state->prevState = currStateMachine->stateMachine->currState;
			state->currState = currStateMachine->stateMachine->nextState;
			state->nextState = NULL;
			state->initialCall = true;
		}

		// If the current state is valid...
		if(state->currState != NULL)
		{
			// If the specified state was found in the table...
			if(state != NULL)
			{
				state->currState->funcPtr(currStateMachine->stateMachine);
				state->initialCall = false;
			}
		}
	}
}
