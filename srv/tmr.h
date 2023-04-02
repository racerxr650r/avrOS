/*
 * tmr.h
 *
 * Header for system timers
 *
 * Created: 4/17/2021 9:12:55 PM
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
#ifndef __TMR_H
#define __TMR_H

// Types ----------------------------------------------------------------------
typedef struct {
	bool		output;
	uint32_t	PT, ET, RT, lastScanCycle, sysTick;
} timer_t;

typedef struct
{
	const char		*name;
	timer_t	*timerState;
} timerDescr_t;

#define ADD_TIMER(tmrName)	static timer_t tmrName; \
							const static timerDescr_t SECTION(TMR_TABLE) CONCAT(tmrName,_descr) = {.name = #tmrName, .timerState = &tmrName};

// External functions ---------------------------------------------------------
extern void tmrReset(timer_t *timer, bool direction);
extern bool tmrFunc(timer_t *timer, uint32_t ticks, bool direction);

// Inline functions -----------------------------------------------------------
static inline uint8_t tmrOnDelay(timer_t *timer, uint32_t ticks)
{
	return(tmrFunc(timer, ticks, false));
}

static inline uint8_t tmrOffDelay(timer_t *timer, uint32_t ticks)
{
	return(tmrFunc(timer, ticks, true));
}
#endif // __TMR_H

