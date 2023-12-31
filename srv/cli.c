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
static int cliNewCmd(volatile fsmStateMachine_t *stateMachine);
static int cliGetKey(volatile fsmStateMachine_t *stateMachine);
static int cliEscKey(volatile fsmStateMachine_t *stateMachine);
static int cliEscSequence(volatile fsmStateMachine_t *stateMachine);
static int cliConsumeKey(volatile fsmStateMachine_t *stateMachine);
static int cliCallCommand(volatile fsmStateMachine_t *stateMachine);
static int cliClearScreen(volatile fsmStateMachine_t *stateMachine);
static int cliWaitTxQueue(volatile fsmStateMachine_t *stateMachine);
static int cliRepeatCommand(volatile fsmStateMachine_t *stateMachine);
static int cliDisplayEscape(volatile fsmStateMachine_t *stateMachine);

// Private Globals ------------------------------------------------------------
//static FILE				cliSTDOUT = FDEV_SETUP_STREAM(uartPutChar, NULL, _FDEV_SETUP_WRITE);

// Command line state machine globals
//#ifdef CLI_SERVICE
static char				key;
static char				commandLine[MAX_CMD_LINE];
static char				previousCommand[MAX_CMD_LINE];
static int				lineCounter = 0;
//#endif // CLI_SERVICE
static char*			argV[MAX_ARGS];
static int				argC;
static commandHandler_t	cmdFuncPtr = NULL;
static cliCommand_t		*currentCommand;

// Start and end of the linker assembled array of cliCommand_t structures
extern void *__start_CLI_CMDS;
extern void *__stop_CLI_CMDS;

// CLI Commands ---------------------------------------------------------------
// Command Help
// Displays the registered command tokens in the system
#ifdef CLI_CLI
// Command help
ADD_COMMAND("help",cliHelp);
ADD_COMMAND("?",cliHelp);
// Command clear
ADD_COMMAND("clear",cliClear);
#endif // CLI_CLI

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

int cliClear(int argC, char *argV[])
{
	// The UNUSED macro prevents the compiler from warning about unused variables
	// Include these only if the function does not use the argC and argV parameters
	UNUSED(argC);
	UNUSED(argV);
	
	// Clear the entire screen	
	// Set the cursor to Home (upper left of screen)
	printf(CLEAR_SCREEN CURSOR_HOME);

	return(0);
}

// CLI State Machine ----------------------------------------------------------

// Initialize the CLI
int cliInit(volatile fsmStateMachine_t *stateMachine)
{
	cliInstance_t *cliInstance = (cliInstance_t*)fsmGetInstance(stateMachine);
	FILE          *inFile = cliInstance->inFile, *outFile = cliInstance->outFile;
	
	INFO("Init CLI");
	
	// Setup stdout so stdout.h functions like printf output the CLI associated
	stdout = outFile;
	stdin = inFile;

    // Display the system greeting
	printf(CLI_BANNER);
	
	fsmSetNextState(stateMachine,cliNewCmd);

	return(0);
}

static int cliNewCmd(volatile fsmStateMachine_t *stateMachine)
{
	fioWaitInput(stdin);
	fsmSetNextState(stateMachine,cliGetKey);
	
	// Display the prompt
	printf("\n" DISPLAY_PROMPT);
	return(0);
}

static int cliGetKey(volatile fsmStateMachine_t *stateMachine)
{
	key = fgetc(stdin);
		
//	if(key!=EOF)
//	{
		if(key == ESC)
		{
			fioWaitInput(stdin);
			fsmSetNextState(stateMachine, cliEscKey);
		}
		else
			fsmSetNextState(stateMachine, cliConsumeKey);
//	}

	return(0);
}

static int cliEscKey(volatile fsmStateMachine_t *stateMachine)
{
	key = fgetc(stdin);

	if(key == '[' || key == 'O')
		fsmSetNextState(stateMachine, cliEscSequence);
	else
		fsmSetNextState(stateMachine, cliGetKey);

	fioWaitInput(stdin);
	return(0);
}

static int cliEscSequence(volatile fsmStateMachine_t *stateMachine)
{
	key = fgetc(stdin);

	if(key == 'A')
	{
		key = KEYCODE_UP;
		fsmSetNextState(stateMachine, cliConsumeKey);
	}
	else if(key == 'D')
	{
		key = KEYCODE_LEFT;
		fsmSetNextState(stateMachine, cliConsumeKey);
	}
	else
	{
		fioWaitInput(stdin);
		fsmSetNextState(stateMachine, cliGetKey);
	}

	return(0);
}

static int cliConsumeKey(volatile fsmStateMachine_t *stateMachine)
{
	switch(key)
	{
		case CR:
			// Transmit a carriage return + line feed back
			printf("\n\r");
			// Update command line and counter
			commandLine[lineCounter] = '\0';
			// Save the previous command
			memcpy(previousCommand,commandLine,lineCounter);
			previousCommand[lineCounter] = '\0';
			lineCounter = 0;
			fsmSetNextState(stateMachine, cliWaitTxQueue);
			break;
		case KEYCODE_LEFT:
		case DEL:
		case BS:
			// If there is some line to delete...
			if(lineCounter>0)
			{
				// Transmit the key back to the connect computer (ECHO OFF)
				--lineCounter;
				commandLine[lineCounter] = 0;
				printf(DISPLAY_PROMPT "%s \b",commandLine);
			}
			fioWaitInput(stdin);
			fsmSetNextState(stateMachine, cliGetKey);
			break;
		case KEYCODE_UP:
			strcpy(commandLine,previousCommand);
			lineCounter = strlen(commandLine);
			printf(DISPLAY_PROMPT "%s",commandLine);
			fioWaitInput(stdin);
			fsmSetNextState(stateMachine, cliGetKey);
			break;
		default:
			// Transmit the key back to the connected computer (ECHO OFF)
			fputc(key,stdout);
			commandLine[lineCounter] = key;
			if(lineCounter<MAX_CMD_LINE-1)
				++lineCounter;
			fioWaitInput(stdin);
			fsmSetNextState(stateMachine, cliGetKey);
			break;
	}
	return(0);
}

static int cliCallCommand(volatile fsmStateMachine_t *stateMachine)
{
	// Attempt to call user command
	cliCallFunction(commandLine);

	// If the repeat command function pointer has been set...
	if(cmdFuncPtr!=NULL)
		fsmSetNextState(stateMachine, cliClearScreen);
	else
		fsmSetNextState(stateMachine, cliNewCmd);

	return(0);
}

static int cliClearScreen(volatile fsmStateMachine_t *stateMachine)
{
	UART_t		*uart = fdev_get_udata(stdout);

	// Wait until the Tx queue is empty...
	if(uartTxEmpty(uart))
	{
		// Clear the entire screen
		printf(CLEAR_SCREEN);
		fsmSetNextState(stateMachine, cliWaitTxQueue);
	}
	
	return(0);
}

static int cliWaitTxQueue(volatile fsmStateMachine_t *stateMachine)
{
	UART_t		*uart = fdev_get_udata(stdout);

	// Wait until the Tx queue is empty...
	if(uartTxEmpty(uart))
	{
		if(fsmGetPreviousState(stateMachine) == cliConsumeKey)
			fsmSetNextState(stateMachine, cliCallCommand);
		else
		{
			// Set the cursor to Home (upper left of screen)
			printf(CURSOR_HOME);
			// Hide the cursor
			printf(CURSOR_HIDE);
			
			fsmSetNextState(stateMachine, cliRepeatCommand);
		}
	}
	
	return(0);
}

static int cliRepeatCommand(volatile fsmStateMachine_t *stateMachine)
{
	// Attempt to call user command
	cmdFuncPtr(argC,argV);
	fsmSetNextState(stateMachine, cliDisplayEscape);
	
	return(0);
}

static int cliDisplayEscape(volatile fsmStateMachine_t *stateMachine)
{
	UART_t		*uart = fdev_get_udata(stdout);

	// Wait until the Tx queue is empty...
	if(uartTxEmpty(uart))
	{
		// Set the cursor to Home (upper left of screen)
		printf("\n\r<<< Press [Ctrl-C] to return to command prompt >>>");
		fsmSetNextState(stateMachine, cliWaitTxQueue);

		// Get the next keystroke
		if(!uartRxEmpty(uart))
		{
			uartReceive(uart,&key,1);

			// If it's the escape key...
			if(key=='\03')
			{
				cmdFuncPtr = NULL;
				// Unhide the cursor
				printf(CURSOR_UNHIDE);
				fsmSetNextState(stateMachine, cliNewCmd);
			}
		}
	}
	
	return(0);
}

// External Functions ---------------------------------------------------------
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
			  if(fsmIsInitialCall())
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
				int ret = currentCommand->funcPtr(argC,argV);
				// If the command returns an error...
				if(ret)
				{
					printf(FG_RED "Command Error Code: %d" FG_DEFAULT,ret);
					WARN("CLI Command Error Code: %d",ret);
				}
				// else no error returned...
				else
					printf(FG_GREEN "OK" FG_DEFAULT);
				
				return(ret);
			  }
          }
    }

    printf(FG_ORANGE "Command not found" FG_DEFAULT);
	WARN("CLI command " BOLD ITALIC "%s" RESET " not found",argV[0]);
    return(-1);
}
