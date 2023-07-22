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
extern void *__start_FSM_TABLE,*__stop_FSM_TABLE;

// Internal Globals -----------------------------------------------------------
static uint32_t scanCycle = 0;
static fsmStateMachine_t   *currStateMachine = NULL, *Ready = NULL, *Wait = NULL, *Stopped = NULL; 
static char initString[] = {"Init"};

// Internal functions ---------------------------------------------------------
static fsmStateMachine_t* fsmGetStateMachine(const char *name);
static int fsmLstAdd(fsmStateMachine_t **list, fsmStateMachine_t *sm);
static int fsmLstRemove(fsmStateMachine_t **list, fsmStateMachine_t *sm);
static void fsmLstPrint(FILE *file, fsmStateMachine_t *list);

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
	
	printf("Ready:\n\r");
	fsmLstPrint(stdout, Ready);
	printf("Wait:\n\r");
	fsmLstPrint(stdout, Wait);
	printf("Stopped:\n\r");
	fsmLstPrint(stdout, Stopped);

	// Walk the table of state machines
/*	fsmStateMachineDescr_t *descr = (fsmStateMachineDescr_t *)&__start_FSM_TABLES;
	for(; descr < (fsmStateMachineDescr_t *)&__stop_FSM_TABLES; ++descr)
	{
		// If no State Machine given, or the specified State Machine is this one...
		if(argc<2 || (argc==2 && !strcmp(descr->name,argv[1])))
		{
			// Display the State Machine Name
			printf("State Machine: %s\n\r",descr->name);
		}
	}*/

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
  if((argc == 2) && (argv[1] != NULL))
  {
    fsmStateMachine_t	*stateMachine = fsmGetStateMachine(argv[1]);
	
    if(stateMachine->currState == NULL && stateMachine != NULL)
      return(fsmNextState(stateMachine,stateMachine->stateMachineDescr->initHandler));
  }
  return(-1);
}

// Internal Functions ----------------------------------------------------------
static int fsmLstAdd(fsmStateMachine_t **list, fsmStateMachine_t *sm)
{
	int ret = 0;
	
	// If sm is valid...
	if(sm != NULL)
	{
		// Start critical section of code
//		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
//		{
			fsmStateMachine_t *curr = *list, *prev = NULL;
			// If there is no elements in the current list...
			if(*list == NULL)
			{
				*list = sm;
				sm->next = NULL;
			}
			// Else there is at least one element in the current list...
			else
			{
				// While this is not the end of the list...
				while(curr!=NULL)
				{
					// If this element is lower priority than the new element...
					if(sm->stateMachineDescr->priority < curr->stateMachineDescr->priority)
					{
						// If not the head element...
						if(prev)
						{
							sm->next = prev->next;
							prev->next = sm;
							break;
						}
						// Else head element...
						else
						{
							sm->next = curr;
							*list = sm;
							break;
						}
					}
					// Step to the next element in the list
					prev = curr;
					curr = curr->next;
				}
				// If reached the end of the list...
				if(curr == NULL)
				{
					prev->next = sm;
					sm->next = NULL;
				}
			}
//		} // End critical section of code
	}
	// Else sm is not valid...
	else
		ret = -1;
	
	return(ret);
}

int fsmLstRemove(fsmStateMachine_t **list, fsmStateMachine_t *sm)
{
	int ret = 0;
	
	// If the state machine is valid...
	if(sm != NULL)
	{
		// Start critical section of code
//		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
//		{
			fsmStateMachine_t *curr = *list, *prev = NULL;
			// Until the end of the list
			while(curr)
			{
				// If this is the element to remove...
				if(curr == sm)
				{
					// If this is not the head...
					if(prev != NULL)
					{
						prev->next = curr->next;
						break;
					}
					// Else this is the head...
					else
					{
						*list = curr->next;
						break;
					}
				}
					
				// Step to the next element
				prev = curr;
				curr = curr->next;
			}
			// If reached the end of the list without finding the element...
			if(curr == NULL)
				ret = -1;
//		} // End critical section of code
	}
	// Else the state machine is not valid...
	else
		ret = false;
		
	return(ret);
}

static void fsmLstPrint(FILE *file, fsmStateMachine_t *list)
{
	fsmStateMachine_t *curr = list;
	
    while(curr)
	{
		fprintf(file, "    %s %d\n\r", curr->stateMachineDescr->name, curr->stateMachineDescr->priority);
		curr = curr->next;
	}
}

static fsmStateMachine_t *fsmGetStateMachine(const char *name)
{
  // Walk the table of state machines
  fsmStateMachineDescr_t *descr = (fsmStateMachineDescr_t *)&__start_FSM_TABLE;
  
  for(; descr < (fsmStateMachineDescr_t *)&__stop_FSM_TABLE; ++descr)
    if(strcmp(name,descr->name) == 0)
      return(descr->stateMachine);

  return(NULL);
}

// External Functions ----------------------------------------------------------
void fsmInit()
{
	// Walk the table of state machines
	fsmStateMachineDescr_t *stateMachineDescr = (fsmStateMachineDescr_t *)&__start_FSM_TABLE;
	for(; stateMachineDescr < (fsmStateMachineDescr_t *)&__stop_FSM_TABLE; ++stateMachineDescr)
	{
		fsmStateMachine_t	*stateMachine = stateMachineDescr->stateMachine;
		// Add the state machine to the ready list in priority order
		fsmLstAdd(&Ready,stateMachine);
	}
}

// Get the scan cycle count
uint32_t fsmScanCycle()
{
	return(scanCycle);
}

// Get the current state machine name
const char* fsmGetCurrentStateMachineName()
{
	const char *ret = NULL;
	
	if(currStateMachine!=NULL)
		ret = currStateMachine->stateMachineDescr->name;
	else
		ret = initString;
	
	return(ret);
}

// The the pointer to the current state machine
fsmStateMachine_t* fsmGetCurrentStateMachine()
{
	return(currStateMachine);
}

void* fsmGetInstance(fsmStateMachine_t *stateMachine)
{
	return(stateMachine->stateMachineDescr->instance);
}

// Is this the first call of the state function after a state change?
bool fsmInitialCall()
{
	return(currStateMachine->initialCall);
}

// Return pointer to the current state
fsmHandler_t fsmCurrentState(fsmStateMachine_t *stateMachine)
{
	return(stateMachine->currState);
}

// Return pointer to the previous state
fsmHandler_t fsmPreviousState(fsmStateMachine_t *stateMachine)
{
	return(stateMachine->prevState);
}

// Change the given state machine to the given state
int fsmNextState(fsmStateMachine_t *stateMachine, fsmHandler_t handler)
{
  if(stateMachine)
    stateMachine->nextState = handler;
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
	}
	else
		return(-1);

	return(0);
}

// Put the given state machine on the wait list
int fsmWait(fsmStateMachine_t *stateMachine)
{
	int ret;
	
    if(!(ret = fsmLstRemove(&Ready,stateMachine)))
		fsmLstAdd(&Wait,stateMachine);
	
	return(ret);
}

// Put the given state machine on the ready list
int fsmReady(fsmStateMachine_t *stateMachine)
{
	int ret;
	
    if(!(ret = fsmLstRemove(&Wait,stateMachine)))
		fsmLstAdd(&Ready,stateMachine);
	
	return(ret);
}


// Finite State Machine Dispatcher. This function steps through the state machine table and calls the current state function for each
void fsmDispatch(void)
{
	++scanCycle;
	
	// Walk the Ready list
	currStateMachine = Ready;
	while(currStateMachine)
	{
		// If a next state has been set, update the current state
		if(currStateMachine->nextState != NULL)
		{
			currStateMachine->prevState = currStateMachine->currState;
			currStateMachine->currState = currStateMachine->nextState;
			currStateMachine->nextState = NULL;
			currStateMachine->initialCall = true;
		}
		// If the current state is valid...
		if(currStateMachine->currState != NULL)
		{
			// If the specified state was found in the table...
			if(currStateMachine != NULL)
			{
				currStateMachine->currState(currStateMachine);
				currStateMachine->initialCall = false;
			}
		}
		// Else the current state is NULL...
		else
		{
		    fsmLstRemove(&Ready,currStateMachine);
		    fsmLstAdd(&Stopped,currStateMachine);
		    INFO("State Machine %s Stopped",currStateMachine->stateMachineDescr->name);
        }
        // Step to the next state machine in the Ready list
        currStateMachine = currStateMachine->next;
	}
	
	// Walk the table of state machines
/*	currStateMachine = (fsmStateMachineDescr_t *)&__start_FSM_TABLES;
	for(; currStateMachine < (fsmStateMachineDescr_t *)&__stop_FSM_TABLES; ++currStateMachine)
	{
		fsmStateMachine_t	*stateMachine = currStateMachine->stateMachine;
	  
		// If a next state has been set, update the current state
		if(stateMachine->nextState != NULL)
		{
			stateMachine->prevState = currStateMachine->stateMachine->currState;
			stateMachine->currState = currStateMachine->stateMachine->nextState;
			stateMachine->nextState = NULL;
			stateMachine->initialCall = true;
		}

		// If the current state is valid...
		if(stateMachine->currState != NULL)
		{
			// If the specified state was found in the table...
			if(stateMachine != NULL)
			{
				stateMachine->currState(currStateMachine->stateMachine);
				stateMachine->initialCall = false;
			}
		}
	}*/
}
