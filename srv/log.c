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

// External Functions ---------------------------------------------------------
// Initialize the CLI and the associated serial port
int logInit(fsmStateMachine_t *stateMachine)
{
	logInstance_t *logInstance = (logInstance_t*)fsmGetInstance(stateMachine);
	FILE          *outFile = logInstance->outFile;
	
	// Setup stderr so stdout.h functions like fprintf output the log
	stderr = outFile;

	// Enable global interrupts
	sei();

	// Hide the cursor
	fprintf(stderr,"\e[?25l");

	// Display the system greeting
	fprintf(stderr,LOG_BANNER);
	
//	fprintf(stderr,"\n\rSystem Memory --------------\n\r");
	// Wait until the Tx queue is empty...
//	memRomStatus(stderr);
//	fprintf(stderr,"\n\r");
	//cliCallFunction("ram");
//	memRamStatus(stderr);

	fsmStop(stateMachine);
	return(0);
}


