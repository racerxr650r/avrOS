/*
 * sys.c
 *
 * Various system functions including system counter/tick
 *
 * Created: 4/11/2021 11:52:48 AM
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
#include "../avrOS.h"

volatile uint32_t	tickOvrflw = 0; 
uint32_t			tickDivisor;

// Command line interface -----------------------------------------------------
#ifdef TICK_CLI
ADD_COMMAND("tick",tickCmd,true);
#endif

int tickCmd(int argc, char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);
	
	uint32_t ticks = sysGetTick();
	uint32_t secs = ticks/1000000;
	uint32_t msecs = (ticks%1000000)/1000;
	
	printf("System Info:\n\r");
	printf("  Tick Timer: %s\n\r",&SYS_TICK_TIMER==&TCB0?"TCB0":&SYS_TICK_TIMER==&TCB1?"TCB1":&SYS_TICK_TIMER==&TCB2?"TCB2":"N/A");
	printf("    CPU Freq: %2lu MHz\n\r",cpuGetFrequency()/FREQ_1_MHZ);
	printf("   Tick Freq: %2lu MHz\n\r",tickDivisor);
	printf(" System Tick: %10lu secs, %10lu msecs\n\r",secs,msecs);

	return(0);
}

// Interrupt Handler ----------------------------------------------------------
ISR(TCB0_INT_vect)
{
	TCB_t *tcb = &SYS_TICK_TIMER;

	// Clear the interrupt
	tcb->INTFLAGS = TCB_CAPT_bm;
	
	// Increment the tick overflow counter
	++tickOvrflw;
	
	// Overflow the tick overflow counter
	// Each overflow tick is equal to 5000 microseconds
	// tickOverflw = 0xffffffff / 50000
//	if(tickOvrflw>858993)
//		tickOvrflw=0;
}

// External Functions ---------------------------------------------------------

// Initialize the system
//
bool sysInit()
{
	// Initialize the system -----------------------------------------------------
	// Setup the internal CPU clock source
	cpuSetOSCHF(CPU_SPEED,false,0);

#ifdef MEM_STATS	
	// Fill the stack area with pattern to detect max stack size
	memStackFill();
#endif	

	// Initialize the system tick counter
	sysInitTick();

	// Initialize the fsm scheduler
	fsmInit();
	
	return(true);
}

void sysInitTick()
{
	TCB_t		*tcb = &SYS_TICK_TIMER;
	uint32_t	freq = cpuGetFrequency();
	
	switch(freq)
	{
		case FREQ_1_MHZ:
			tickDivisor = 1;
			break;
		case FREQ_2_MHZ:
			tickDivisor = 2;
			break;
		case FREQ_3_MHZ:
			tickDivisor = 3;
			break;
		case FREQ_4_MHZ:
			tickDivisor = 2;
			break;
		case FREQ_8_MHZ:
			tickDivisor = 4;
			break;
		case FREQ_12_MHZ:
			tickDivisor = 6;
			break;
		case FREQ_16_MHZ:
			tickDivisor = 8;
			break;
		case FREQ_20_MHZ:
			tickDivisor = 10;
			break;
		case FREQ_24_MHZ:
			tickDivisor = 12;
			break;
		default:
			return;
	}

	// Disable interrupts while setting up timer registers
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// Setup control reg A (Peripheral clock DIV 2)
		if(freq>FREQ_3_MHZ)
			tcb->CTRLA = TCB_CLKSEL_DIV2_gc;
		else
			tcb->CTRLA = TCB_CLKSEL_DIV1_gc;
		// Set the top to 60,000
		tcb->CCMP = (uint16_t)TICK_TOP;
		// Enable the overflow interrupt
		tcb->INTCTRL |= TCB_CAPT_bm;
		// Setup control reg B (Periodic timer mode)
		tcb->CTRLB = TCB_CNTMODE_INT_gc;
		// Enable the clock
		tcb->CTRLA |= TCB_ENABLE_bm;
	}
}

uint32_t sysGetTick()
{
	TCB_t *tcb = &SYS_TICK_TIMER;
	uint32_t ret;
	
	// Add the low order byte of the current counter
	ret = tcb->CNT / tickDivisor;
	// Add the high order byte of the current counter
	//ret = ret + (tcb->CNTH*256);
	// Calculate and add the total for the tick overflow counter
	ret = ret + (tickOvrflw*(TICK_TOP/tickDivisor));
	
	return(ret);
}

// Put the system to sleep until the next interrupt
void sysSleep()
{
	return;
}
