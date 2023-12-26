/*
 * log.c
 *
 * Created: 4/25/2021 6:01:41 PM
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

// Internal Functions ----------------------------------------------------------
static int logRAM(volatile fsmStateMachine_t *stateMachine)
{
	// Display the system RAM info
	memRamStatus(stderr);

	// Stop the log initialization SM
	fsmStop(stateMachine);
	return(0);
}

static int logROM(volatile fsmStateMachine_t *stateMachine)
{
	// Display the system ROM info
	//fprintf(stderr,"\n\rSystem Memory --------------\n\r");
	memRomStatus(stderr);
	fprintf(stderr,"\n\r");

	// Wait until the stderr output queue is empty and then go to next state
    fioWaitOutput(stderr);
	fsmSetNextState(stateMachine,logRAM);

	return(0);
}

// External Functions ----------------------------------------------------------
int logInit(volatile fsmStateMachine_t *stateMachine)
{
	logInstance_t *logInstance = (logInstance_t*)fsmGetInstance(stateMachine);
	FILE          *outFile = logInstance->outFile;
	
	// Setup stderr so stdout.h functions like fprintf output the log
	stderr = outFile;

	// Hide the cursor
	fprintf(stderr,"\e[?25l");
	// Display the system greeting
	fprintf(stderr,LOG_BANNER);

	// Wait until the stderr output queue is empty and then go to next state
    fioWaitOutput(stderr);
    fsmSetNextState(stateMachine,logROM);

	return(0);
}
