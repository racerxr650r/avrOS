/*
 * log.c
 *
 * Implements logging to file I/O stream
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
int logInit(const fsmStateMachineDescr_t *stateMachineDescr)
{
	logInstance_t *logInstance = (logInstance_t*)initGetInstance(stateMachineDescr);
	FILE          *outFile = logInstance->outFile;
	
	// Setup stderr so stdout.h functions like fprintf output the log
	stderr = outFile;

	// Hide the cursor
	fprintf(stderr,CURSOR_HIDE);
	// Display the system greeting
	fprintf(stderr,LOG_BANNER);

	return(0);
}

// External Function ----------------------------------------------------------
void logRam()
{
	if(stderr!=NULL)
	{
		memRamStatus(stderr);
	}
}

void logRom()
{
	if(stderr!=NULL)
	{
		memRomStatus(stderr);
	}
}

void logNewLine()
{
	if(stderr!=NULL)
	{
		fprintf(stderr,"\n\r");
	}
}
