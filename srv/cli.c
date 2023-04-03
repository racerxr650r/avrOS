/*
 * cli.c
 *
 * Implements CLI manager and API
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Private functions prototypes ----------------------------------------------
static const char *cliCallCommandName();

// Private Globals ------------------------------------------------------------
static FILE				cliSTDOUT = FDEV_SETUP_STREAM(uartPutChar, NULL, _FDEV_SETUP_WRITE);

// Command line state machine globals
#ifdef CLI_SERVICE
static char				key;
static char				commandLine[MAX_CMD_LINE];
static char				previousCommand[MAX_CMD_LINE];
static int				lineCounter = 0;
#endif // CLI_SERVICE
static char*			argV[MAX_ARGS];
static int				argC;
static commandHandler_t	cmdFuncPtr = NULL;
static cliCommand_t		*currentCommand;

// Start and end of the linker assembled array of cliCommand_t structures
extern void *__start_CLI_CMDS;
extern void *__stop_CLI_CMDS;

// OS Objects -----------------------------------------------------------------
#ifdef CLI_SERVICE
// Create the Tx and Rx queues
ADD_QUEUE(cliRxQueue,CLI_RX_QUEUE_SIZE);
ADD_QUEUE(cliTxQueue,CLI_TX_QUEUE_SIZE);
// Create the Uart instance for CLI I/O
ADD_UART(cliUart,CLI_USART,&cliTxQueue,&cliRxQueue);
#endif

// CLI Commands ---------------------------------------------------------------
// Command Help
// Displays the registered command tokens in the system
#ifdef CLI_CLI
ADD_COMMAND("help",cliHelp);
ADD_COMMAND("?",cliHelp);

int cliHelp(int argC, char *argV[])
{
	// The UNUSED macro prevents the compiler from warning about unused variables
	// Include these only if the function does not use the argC and argV parameters
	UNUSED(argC);
	UNUSED(argV);
	cliCommand_t *cmd = (cliCommand_t *)&__start_CLI_CMDS;

	for(; cmd < (cliCommand_t *)&__stop_CLI_CMDS; ++cmd)
		printf("    %s%s\n\r",cmd->commandStr,cmd->repeatable?" (-r)":"");

	return(0);
}

// Command Clear
ADD_COMMAND("clear",cliClear);

int cliClear(int argC, char *argV[])
{
	// The UNUSED macro prevents the compiler from warning about unused variables
	// Include these only if the function does not use the argC and argV parameters
	UNUSED(argC);
	UNUSED(argV);
	
	// Clear the entire screen	
	printf("\e[2J");
	// Set the cursor to Home (upper left of screen)
	printf("\e[H");

	return(0);
}
#endif // CLI_CLI

// CLI State Machine ----------------------------------------------------------
#ifdef CLI_SERVICE
// Create instance of the cli state machine
ADD_STATE_MACHINE(cliStateMachine, cliNewCmd, false);
// Create initial state of the cli state machine
ADD_STATE(cliStateMachine, cliNewCmd);
// Create get key state of the cli state machine
ADD_STATE(cliStateMachine, cliGetKey);
// Create escape key state of the cli state machine
ADD_STATE(cliStateMachine, cliEscKey);
// Create escape sequence state of the cli state machine
ADD_STATE(cliStateMachine, cliEscSequence);
// Create keystroke state of the cli state machine
ADD_STATE(cliStateMachine, cliConsumeKey);
// Create call command state of the cli state machine
ADD_STATE(cliStateMachine, cliCallCommand,cliCallCommandName);
// Create wait Tx queue state of the cli state machine
ADD_STATE(cliStateMachine, cliClearScreen);
// Create wait Tx queue state of the cli state machine
ADD_STATE(cliStateMachine, cliWaitTxQueue);
// Create repeat command state of the cli state machine
ADD_STATE(cliStateMachine, cliRepeatCommand);
// Create repeat command state of the cli state machine
ADD_STATE(cliStateMachine, cliDisplayEscape);

int cliNewCmdFunc(fsmStateMachine_t *stateMachine)
{
	// Display the prompt
	printf("\n\r> ");
	fsmNextState(stateMachine,&cliGetKey);
	return(0);
}

int cliGetKeyFunc(fsmStateMachine_t *stateMachine)
{
	UART_t		*uart = fdev_get_udata(stdout);
	
	if(!uartRxEmpty(uart))
	{
		uartReceive(uart,&key,1);
		
		if(key == '\e')
			fsmNextState(stateMachine, &cliEscKey);
		else
			fsmNextState(stateMachine, &cliConsumeKey);
	}
	return(0);
}

int cliEscKeyFunc(fsmStateMachine_t *stateMachine)
{
	UART_t		*uart = fdev_get_udata(stdout);
	
	if(!uartRxEmpty(uart))
	{
		uartReceive(uart,&key,1);

		if(key == '[' || key == 'O')
			fsmNextState(stateMachine, &cliEscSequence);
		else
			fsmNextState(stateMachine, &cliGetKey);
	}
	return(0);
}

int cliEscSequenceFunc(fsmStateMachine_t *stateMachine)
{
	UART_t		*uart = fdev_get_udata(stdout);
	
	if(!uartRxEmpty(uart))
	{
		uartReceive(uart,&key,1);

		if(key == 'A')
		{
			key = KEYCODE_UP;
			fsmNextState(stateMachine, &cliConsumeKey);
		}
		else if(key == 'D')
		{
			key = KEYCODE_LEFT;
			fsmNextState(stateMachine, &cliConsumeKey);
		}
		else
		fsmNextState(stateMachine, &cliGetKey);
	}
	return(0);
}

int cliConsumeKeyFunc(fsmStateMachine_t *stateMachine)
{
	switch(key)
	{
		case '\r':
			// Transmit a carriage return + line feed back
			printf("\n\r");
			// Update command line and counter
			commandLine[lineCounter] = '\0';
			// Save the previous command
			memcpy(previousCommand,commandLine,lineCounter);
			previousCommand[lineCounter] = '\0';
			lineCounter = 0;
			fsmNextState(stateMachine,&cliWaitTxQueue);
			break;
		case KEYCODE_LEFT:
		case '\b':
			// If there is some line to delete...
			if(lineCounter>0)
			{
				// Transmit the key back to the connect computer (ECHO OFF)
				--lineCounter;
				commandLine[lineCounter] = 0;
				printf("\r> %s \b",commandLine);
			}
			fsmNextState(stateMachine,&cliGetKey);
			break;
		case KEYCODE_UP:
			strcpy(commandLine,previousCommand);
			lineCounter = strlen(commandLine);
			printf("\r> %s",commandLine);
			fsmNextState(stateMachine,&cliGetKey);
			break;
		default:
			// Transmit the key back to the connected computer (ECHO OFF)
			putchar(key);
			commandLine[lineCounter] = key;
			if(lineCounter<MAX_CMD_LINE-1)
			++lineCounter;
			fsmNextState(stateMachine,&cliGetKey);
			break;
	}
	return(0);
}

int cliCallCommandFunc(fsmStateMachine_t *stateMachine)
{
	// Attempt to call user command
	cliCallFunction(commandLine);

	// If the repeat command function pointer has been set...
	if(cmdFuncPtr!=NULL)
		fsmNextState(stateMachine,&cliClearScreen);
	else
		fsmNextState(stateMachine,&cliNewCmd);

	return(0);
}

int cliClearScreenFunc(fsmStateMachine_t *stateMachine)
{
	UART_t		*uart = fdev_get_udata(stdout);

	// Wait until the Tx queue is empty...
	if(uartTxEmpty(uart))
	{
		// Clear the entire screen
		printf("\e[2J");
		fsmNextState(stateMachine,&cliWaitTxQueue);
	}
	
	return(0);
}

int cliWaitTxQueueFunc(fsmStateMachine_t *stateMachine)
{
	UART_t		*uart = fdev_get_udata(stdout);

	// Wait until the Tx queue is empty...
	if(uartTxEmpty(uart))
	{
		if(fsmPreviousState(stateMachine) == &cliConsumeKey)
			fsmNextState(stateMachine,&cliCallCommand);
		else
		{
			// Set the cursor to Home (upper left of screen)
			printf("\e[H");
			// Hide the cursor
			printf("\e[?25l");
			
			fsmNextState(stateMachine,&cliRepeatCommand);
		}
	}
	
	return(0);
}

int cliRepeatCommandFunc(fsmStateMachine_t *stateMachine)
{
	// Attempt to call user command
	cmdFuncPtr(argC,argV);
	fsmNextState(stateMachine,&cliDisplayEscape);
	
	return(0);
}

int cliDisplayEscapeFunc(fsmStateMachine_t *stateMachine)
{
	UART_t		*uart = fdev_get_udata(stdout);

	// Wait until the Tx queue is empty...
	if(uartTxEmpty(uart))
	{
		// Set the cursor to Home (upper left of screen)
		printf("\n\r<<< Press [Ctrl-C] to return to command prompt >>>");
		fsmNextState(stateMachine,&cliWaitTxQueue);

		// Get the next keystroke
		if(!uartRxEmpty(uart))
		{
			uartReceive(uart,&key,1);

			// If it's the escape key...
			if(key=='\03')
			{
				cmdFuncPtr = NULL;
				// Unhide the cursor
				printf("\e[?25h");
				fsmNextState(stateMachine,&cliNewCmd);
			}
		}
	}
	
	return(0);
}
#endif  // CLI_SERVICE

// Private Functions ----------------------------------------------------------
const char* cliCallCommandName()
{
	return(currentCommand->commandStr);
}

// External Functions ---------------------------------------------------------
// Initialize the CLI and the associated serial port
void cliInit()
{
	INFO("Init CLI using %s at %lu bps",&CLI_USART==&USART0?"USART0":&CLI_USART==&USART1?"USART1":&CLI_USART==&USART2?"USART2":"N/A",CLI_BAUDRATE);
	
	// Setup stdout so stdout.h functions like printf output the CLI associated
	// uart or virtual uart
	fdev_set_udata(&cliSTDOUT,(void *)&cliUart);
	stdout = &cliSTDOUT;
	// Initialize CLI UART
	uartInit(&cliUart,CLI_BAUDRATE,CLI_PARITY,CLI_DATA_BITS,CLI_STOP_BITS);

	// Enable global interrupts
	sei();

    // Display the system greeting
	printf(CLI_BANNER);
	
	printf("\n\rSystem Memory --------------\n\r");
	// Wait until the Tx queue is empty...
	while(!uartTxEmpty(&cliUart));
	//cliCallFunction("rom");
	memRomStatus(stdout);
	printf("\n\r");
	// Wait until the Tx queue is empty...
	while(!uartTxEmpty(&cliUart));
	//cliCallFunction("ram");
	memRamStatus(stdout);
	// Wait until the Tx queue is empty...
	while(!uartTxEmpty(&cliUart));
}

// Parse command line and call associated function
int cliCallFunction(char *commandLine)
{
    bool  newCommand=true;
	bool  repeatCommand = false;
	
	argC = 0;

    if(strlen(commandLine))
    {
      // Parse up the command line into argC and argV format
      for(int i=0;commandLine[i]!=0 && i<MAX_CMD_LINE;++i)
      {
		  // If the repeat command switch is set...
		  if((commandLine[i]=='-' && (commandLine[i+1]==(char)tolower(REPEAT_SWITCH) || commandLine[i+1]==(char)toupper(REPEAT_SWITCH)) && (commandLine[i+2]==' ' || commandLine[i+2]=='\t' || commandLine[i+2]==0)))
		  {
				repeatCommand = true;
				i += 1;
				newCommand = false;
		  }
          else if(newCommand==true && (commandLine[i]!= ' ' && commandLine[i]!='\t'))
          {
              argV[argC++] = &commandLine[i];
              newCommand = false;
          }
          else if(commandLine[i]==' ' || commandLine[i]=='\t')
          {
              commandLine[i] = 0;
              newCommand = true;
          }
      }

      // Scan the command function table for the provided command (argV[0])
      currentCommand = (cliCommand_t *)&__start_CLI_CMDS;
      for(; currentCommand < (cliCommand_t *)&__stop_CLI_CMDS; ++currentCommand)
          if(!strcmp(argV[0],currentCommand->commandStr))
          {
			  // If this is the initial scan cycle of this state...
			  if(fsmInitialCall())
				  INFO("CLI command %s",currentCommand->commandStr);

			  // If the repeat flag was found and this command is repeatable...
			  if(repeatCommand && currentCommand->repeatable)
			  {
				  cmdFuncPtr=currentCommand->funcPtr;
				  return(0);
			  }
			  // Command is not repeating
			  else
			  {
				// Call the command handler function
				return(currentCommand->funcPtr(argC,argV));
			  }
          }
    }

    printf("\n\rCommand not found");
    return(-1);
}
