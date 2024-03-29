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
volatile static fsmStateMachine_t   *currStateMachine = NULL, *Ready = NULL, *Wait = NULL, *Stopped = NULL; 
static char initString[] = {"Init"};

// Internal functions ---------------------------------------------------------
volatile static fsmStateMachine_t* fsmGetStateMachine(const char *name);
static int fsmLstAdd(volatile fsmStateMachine_t **list, volatile fsmStateMachine_t *sm);
static int fsmLstRemove(volatile fsmStateMachine_t **list, volatile fsmStateMachine_t *sm);
static bool fsmListFind(volatile fsmStateMachine_t *list, volatile fsmStateMachine_t *sm);
static void fsmLstPrint(FILE *file, volatile fsmStateMachine_t *list);
static void initTablePrint(FILE *file);

// CLI Commands ---------------------------------------------------------------
#ifdef FSM_CLI
ADD_COMMAND("fsm",fsmCmd,true);
static int fsmCmd(int argc, char *argv[])
{
	int ret = 0;

	if(argc == 1)
	{
		printf(BOLD UNDERLINE FG_BLUE "Device Initializers:\n\r" RESET);
		initTablePrint(stdout);
		
		printf(BOLD UNDERLINE FG_BLUE "Ready Queue:\n\r" RESET);
		fsmLstPrint(stdout, Ready);
		printf(BOLD UNDERLINE FG_BLUE "Wait Queue:\n\r" RESET);
		fsmLstPrint(stdout, Wait);
		printf(BOLD UNDERLINE FG_BLUE "Stopped Queue:\n\r" RESET);
		fsmLstPrint(stdout, Stopped);
	}
	else if((argc == 2) && (argv[1] != NULL))
	{
		volatile fsmStateMachine_t *stateMachine = fsmGetStateMachine(argv[1]);
		
		if(stateMachine!=NULL)
		{
			printf(BOLD UNDERLINE FG_BLUE "%-20s Priority:%4d Instance: %s\n\r" RESET,	stateMachine->stateMachineDescr->name,
																					stateMachine->stateMachineDescr->priority,
																					stateMachine->stateMachineDescr->instance!=NULL?"Yes":"No");
			printf("Prev State: %-20s Curr State: %-20s Next State: %-20s\n\r",	stateMachine->prevStateName,
																				stateMachine->currStateName,
																				stateMachine->nextStateName);
			printf("Initial State: %s Ticks: %7lu\n\r",stateMachine->initialCall==true?"Yes":"No",stateMachine->ticks);
			printf("Run State: ");
			if(fsmListFind(Ready,stateMachine) == true)
				printf("Ready");
			else if(fsmListFind(Wait,stateMachine) == true)
				printf("Waiting");
			else if(fsmListFind(Stopped,stateMachine) == true)
				printf("Stopped");
			printf("\n\r");
		}
		else
			ret = -1;
	}

	return(ret);
}

ADD_COMMAND("fsmStop",fsmStopCmd);
static int fsmStopCmd(int argc, char *argv[])
{
	// If command line includes a name of a state machine...
	if((argc == 2) && (argv[1] != NULL))
	{
		volatile fsmStateMachine_t *stateMachine = fsmGetStateMachine(argv[1]);
		
		if(stateMachine!=NULL)
			return(fsmStop(stateMachine));
	}
	return(-1);
}

ADD_COMMAND("fsmStart",fsmStartCmd);
static int fsmStartCmd(int argc, char *argv[])
{
  // If command line includes a name of a state machine...
  if((argc == 2) && (argv[1] != NULL))
  {
    volatile fsmStateMachine_t	*stateMachine = fsmGetStateMachine(argv[1]);
	
    if(stateMachine != NULL)
		return(fsmReady(stateMachine));
  }
  return(-1);
}

ADD_COMMAND("fsmReset",fsmResetCmd);
static int fsmResetCmd(int argc, char *argv[])
{
  // If command line includes a name of a state machine...
  if((argc == 2) && (argv[1] != NULL))
  {
    volatile fsmStateMachine_t	*stateMachine = fsmGetStateMachine(argv[1]);
	
    if(stateMachine != NULL)
	{
		stateMachine->nextState = stateMachine->stateMachineDescr->handler.fsmHandler;
		return(0);
	}
  }
  return(-1);
}
#endif // FSM_CLI

// Internal Functions ----------------------------------------------------------
static int fsmLstAdd(volatile fsmStateMachine_t **list, volatile fsmStateMachine_t *sm)
{
	int ret = 0;
	
	// If sm is valid...
	if(sm != NULL)
	{
		volatile fsmStateMachine_t *curr = *list, *prev = NULL;
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
	}
	// Else sm is not valid...
	else
		ret = -1;
	
	return(ret);
}

static int fsmLstRemove(volatile fsmStateMachine_t **list, volatile fsmStateMachine_t *sm)
{
	int ret = 0;
	
	// If the state machine is valid...
	if(sm != NULL)
	{
		volatile fsmStateMachine_t *curr = *list, *prev = NULL;
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
				curr->next = NULL;
			}
				
			// Step to the next element
			prev = curr;
			curr = curr->next;
		}
		// If reached the end of the list without finding the element...
		if(curr == NULL)
			ret = -1;
	}
	// Else the state machine is not valid...
	else
		ret = false;
		
	return(ret);
}

static bool fsmListFind(volatile fsmStateMachine_t *list, volatile fsmStateMachine_t *stateMachine)
{
	while(list)
	{
		if(list == stateMachine)
			return(true);
		list = list->next;
	}
	return(false);
}
static void fsmLstPrint(FILE *file, volatile fsmStateMachine_t *list)
{
	volatile fsmStateMachine_t *curr = list;

	if(curr==NULL)
		fprintf(file,"\t<none>\n\r");
	else
		while(curr)
		{
			fprintf(file, "\t%-20s", curr->stateMachineDescr->name);
			fprintf(file, "Priority:%4d Ticks:%6lu\n\r", curr->stateMachineDescr->priority,curr->ticks);
			curr = curr->next;
		}
}

volatile static fsmStateMachine_t *fsmGetStateMachine(const char *name)
{
  // Walk the table of state machines
  fsmStateMachineDescr_t *descr = (fsmStateMachineDescr_t *)&__start_FSM_TABLE;
  
  for(; descr < (fsmStateMachineDescr_t *)&__stop_FSM_TABLE; ++descr)
    if(strcmp(name,descr->name) == 0)
      return(descr->stateMachine);

  return(NULL);
}

static void initTablePrint(FILE *file)
{
	// Walk the table of state machines backwards
  	fsmStateMachineDescr_t *descr = (fsmStateMachineDescr_t *)&__stop_FSM_TABLE-1;
  
	for(; descr >= (fsmStateMachineDescr_t *)&__start_FSM_TABLE; --descr)
		if(descr->stateMachine == NULL)
			fprintf(file,"\t%s\n\r",descr->name);
}

// External Functions ----------------------------------------------------------
// Initialize the state machine manager
void fsmInit()
{
	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// Walk the table of state machines backwards so the initializers are
		// in the order they were added in the source files
		fsmStateMachineDescr_t *stateMachineDescr = (fsmStateMachineDescr_t *)&__stop_FSM_TABLE-1;
		for(; stateMachineDescr >= (fsmStateMachineDescr_t *)&__start_FSM_TABLE; --stateMachineDescr)
		{
			volatile fsmStateMachine_t	*stateMachine = stateMachineDescr->stateMachine;

			// If this is a state machine...
			if(stateMachine != NULL)
			{
				// Add the state machine to the ready list in priority order
				fsmLstAdd(&Ready,stateMachine);
			}
			// Else this is an initializer
			else
			{
				// Call the initializer
				stateMachineDescr->handler.initHandler(stateMachineDescr);
			}
		}
	} // End of critical section

	// Enable global interrupts
	sei();
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

// Get the current state machine
volatile fsmStateMachine_t* fsmGetCurrentStateMachine()
{
	return(currStateMachine);
}

// Get the state machine instance
void* fsmGetInstance(volatile fsmStateMachine_t *stateMachine)
{
	return(stateMachine->stateMachineDescr->instance);
}

// Get the state machine instance
void* initGetInstance(const fsmStateMachineDescr_t *stateMachineDescr)
{
	return(stateMachineDescr->instance);
}

// Initial call to the current state?
bool fsmIsInitialCall()
{
	return(currStateMachine->initialCall);
}

// Get the name of the current state machine state
const char* fsmGetCurrentStateName(volatile fsmStateMachine_t *stateMachine)
{
	return(stateMachine->currStateName);
}

// Get pointer to the current state machine state (function)
fsmHandler_t fsmGetCurrentState(volatile fsmStateMachine_t *stateMachine)
{
	return(stateMachine->currState);
}

// Get the name of the previous state machine state
const char* fsmGetPreviousStateName(volatile fsmStateMachine_t *stateMachine)
{
	return(stateMachine->prevStateName);
}

// Get pointer to the previous state
fsmHandler_t fsmGetPreviousState(volatile fsmStateMachine_t *stateMachine)
{
	return(stateMachine->prevState);
}

// Set the next state of the given state machine
int fsmSetNextStateBasic(volatile fsmStateMachine_t *stateMachine,
						fsmHandler_t handler)
{
	if(stateMachine)
		stateMachine->nextState = handler;
	else
		return(-1);

	return(0);
}

// Set the next state of the given state machine
int fsmSetNextStateVerbose(volatile fsmStateMachine_t *stateMachine,
							fsmHandler_t handler,
							const char *name)
{
	if(stateMachine)
	{
		stateMachine->nextState = handler;
		stateMachine->nextStateName = name;
	}
	else
		return(-1);

	return(0);
}

// Stop the given state machine
int fsmStop(volatile fsmStateMachine_t *stateMachine)
{
	int ret;
	
	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if(!(ret = fsmLstRemove(&Ready,stateMachine)))
			fsmLstAdd(&Stopped,stateMachine);
		else if(!(ret = fsmLstRemove(&Wait,stateMachine)))
			fsmLstAdd(&Stopped,stateMachine);
	} // End critical section
		
	return(ret);
}

// Put the given state machine on the wait list
int fsmWait(volatile fsmStateMachine_t *stateMachine)
{
	int ret;
	
	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if(!(ret = fsmLstRemove(&Ready,stateMachine)))
			fsmLstAdd(&Wait,stateMachine);
	} // End critical section
	
	return(ret);
}

// Put the given state machine on the ready list
int fsmReady(volatile fsmStateMachine_t *stateMachine)
{
	int ret;
	
	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if(!(ret = fsmLstRemove(&Wait,stateMachine)))
			fsmLstAdd(&Ready,stateMachine);
		else if(!(ret = fsmLstRemove(&Stopped,stateMachine)))
			fsmLstAdd(&Ready,stateMachine);
		else
			ret = -1;
	} // End critical section
	
	return(ret);
}

// Update the tick counters of the state machines in the Wait queue
void fsmUpdateWaitTicks()
{
	volatile fsmStateMachine_t *smCurrent = Wait;

	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		while(smCurrent)
		{
			if(smCurrent->ticks)
			{
				--smCurrent->ticks;
				if(!smCurrent->ticks)
					fsmReady(smCurrent);
			}
			smCurrent = smCurrent->next;
		}
	}
}

// Set the state machine to sit in the wait queue for x ticks
void fsmWaitTicks(volatile fsmStateMachine_t*stateMachine, uint32_t ticks)
{
	stateMachine->ticks = ticks;
	fsmWait(stateMachine);
}

// Set the state machine to sit in the wait queue for x milliseconds
void fsmWaitMilliseconds(volatile fsmStateMachine_t*stateMachine, uint16_t ms)
{
	uint16_t freq = sysGetTickFreq();

	fsmWaitTicks(stateMachine,(freq/1000)*ms);
}

// Finite State Machine Dispatcher. This function steps through the state machine table and calls the current state function for each
void fsmDispatch(void)
{
	++scanCycle;
	
	// Resolve the triggered events
  	evntDispatch();

	while(Ready != NULL)
	{
		// Walk the Ready list
		currStateMachine = Ready;
		while(currStateMachine)
		{
			// Get the next state machine in the Ready list
			volatile fsmStateMachine_t *nextStateMachine = currStateMachine->next;				

			// If a next state has been set, update the current state...
			if(currStateMachine->nextState != NULL)
			{
				currStateMachine->prevState = currStateMachine->currState;
				currStateMachine->currState = currStateMachine->nextState;
				currStateMachine->nextState = NULL;
				currStateMachine->initialCall = true;
				currStateMachine->prevStateName = currStateMachine->currStateName;
				currStateMachine->currStateName = currStateMachine->nextStateName;
				currStateMachine->nextStateName = NULL;
			}

			// If the current state is valid..
			if(currStateMachine->currState != NULL)
			{
				// Call the current state machine's state handler
				currStateMachine->currState(currStateMachine);
				currStateMachine->initialCall = false;
			}
			
			// Assign the next state machine in the Ready list
			currStateMachine = nextStateMachine;
 
     	    // If an event has occurred...
  	        if(evntDispatch())
  	            // Reset to the highest priority task
				currStateMachine = Ready;
		}
	}
}
