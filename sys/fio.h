/*
 * fio.h
 *
 * Type and macros to augment file stream io
 *
 * Created: 7/14/2023 4:14:29 PM
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
#ifndef __FIO_H
#define __FIO_H

// Includes -------------------------------------------------------------------
#include "../avrOS.h"

// Types ----------------------------------------------------------------------
typedef struct
{
	volatile queue_t		*input, *output;
}fioBuffers_t;

// Macros ----------------------------------------------------------------------
#define fioGetInputQueue(file)	     (((ioBuffers_t)file.buf)->input)
#define fioGetOutputQueue(file)      (((ioBuffers_t)file.buf)->output)
#define fioSetInputQueue(file, que)  ((ioBuffers_t)file.buf)->input = que;
#define fioSetOutputQueue(file, que) ((ioBuffers_t)file.buf)->input = que;

// Wait until the file input buffer is not empty
static inline void fioWaitInput(FILE *file)
{
	fioBuffers_t *buffer = (fioBuffers_t *)(file->buf);
	
	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		evntEnable(queGetEvent(buffer->input), QUE_EVENT_NOT_EMPTY, fsmReady, fsmGetCurrentStateMachine());
		fsmWait(fsmGetCurrentStateMachine());
	} // End of critical section
}

// Wait until the file output buffer is empty
static inline void fioWaitOutput(FILE *file)
{
	fioBuffers_t *buffer = (fioBuffers_t *)(file->buf);
	
	// Start critical section of code
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		evntEnable(queGetEvent(buffer->output), QUE_EVENT_EMPTY, fsmReady, fsmGetCurrentStateMachine());
		fsmWait(fsmGetCurrentStateMachine());
	} // End of critical section
}

static inline void fioBusyWaitInput(FILE *file)
{
	fioBuffers_t *buffer = (fioBuffers_t *)(file->buf);

	while(queIsEmpty(buffer->input) == true);
}

static inline void fioBusyWaitOutput(FILE *file)
{
	fioBuffers_t *buffer = (fioBuffers_t *)(file->buf);

	while(queIsEmpty(buffer->output) == false);
}

#endif  // __FIO_H
