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

// Globals --------------------------------------------------------------------
static volatile uint32_t	sysTicks = 0;

// Event for system timer ticks to update the waiting state machines
ADD_EVENT(tick);

// Interrupt Handler ----------------------------------------------------------
#if SYS_TICK_TIMER==SYS_TIMER_TCB0
#define SYS_TICK_INT_VECT TCB0_INT_vect
#elif SYS_TICK_TIMER==SYS_TIMER_TCB1
#define SYS_TICK_INT_VECT TCB1_INT_vect
#elif SYS_TICK_TIMER==SYS_TIMER_TCB2
#define SYS_TICK_INT_VECT TCB1_INT_vect
#endif

ISR(SYS_TICK_INT_VECT)
{
	// Clear the interrupt
	(*(TCB_t*)SYS_TICK_TIMER).INTFLAGS = TCB_CAPT_bm;
	
	// Increment the tick counter
	++sysTicks;

	// Trigger the timer update event
	evntTrigger(&tick,EVENT_TYPE_TICK);	
}

// Command line interface -----------------------------------------------------
#ifdef SYS_CLI
ADD_COMMAND("tick",tickCmd,true);
ADD_COMMAND("tickFreq",tickFreqCmd);
#endif

int tickCmd(int argc, char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);
	
	uint32_t ticks = sysGetTickCount();
	uint32_t secs = ticks/1000;
	
	printf(BOLD FG_BLUE "  Tick Timer: " RESET "%s\n\r",SYS_TICK_TIMER==SYS_TIMER_TCB0?"TCB0":SYS_TICK_TIMER==SYS_TIMER_TCB1?"TCB1":SYS_TICK_TIMER==SYS_TIMER_TCB2?"TCB2":"N/A");
	printf(BOLD FG_BLUE "    CPU Freq: " RESET "%10u MHz\n\r",cpuGetFrequency()/1000);
	printf(BOLD FG_BLUE "   Tick Freq: " RESET "%10u kHz\n\r",sysGetTickFreq());
	printf(BOLD FG_BLUE " System Tick: " RESET "%10lu secs\n\r",secs);
	printf(BOLD FG_BLUE " System Tick: " RESET "%10lu cnt\n\r",ticks);

	return(0);
}

int tickFreqCmd(int argc, char *argv[])
{
	int ret = -1;

	if(argc == 2)
	{
		uint16_t freq = atoi(argv[1]);

		if(freq >= 1000)
		{
			sysSetTickFreq(freq);
			ret = 0;
		}
	}

	return(ret);
}

// Internal Functions ---------------------------------------------------------
int sysUpdateWaitTicks(volatile fsmStateMachine_t *sm)
{
	fsmUpdateWaitTicks();
	evntEnable(&tick,EVENT_TYPE_TICK,sysUpdateWaitTicks,NULL);
	return(0);
}

void sysInitTick(TCB_t *tcb, uint16_t sysTickFreq)
{
	uint32_t		cpuFreq = cpuGetFrequency();
	uint16_t		tickDivisor;
	TCB_CLKSEL_t	clockSource;

	sysTickFreq = sysTickFreq/1000;

	if(cpuFreq==1000)
	{
		tickDivisor = 1000/sysTickFreq;
		clockSource = TCB_CLKSEL_DIV1_gc;
	}
	else
	{
		tickDivisor = cpuFreq/(2*sysTickFreq);
		clockSource = TCB_CLKSEL_DIV2_gc;
	}

	// Disable interrupts while setting up timer registers
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// Setup control reg A (Peripheral clock DIV 2)
		tcb->CTRLA = clockSource;
		// Set the top to tick divisor for tick freq
		tcb->CCMP = tickDivisor;
		// Enable the overflow interrupt
		tcb->INTCTRL |= TCB_CAPT_bm;
		// Setup control reg B (Periodic timer mode)
		tcb->CTRLB = TCB_CNTMODE_INT_gc;
		// Enable the clock
		tcb->CTRLA |= TCB_ENABLE_bm;
	}

	evntEnable(&tick,EVENT_TYPE_TICK,sysUpdateWaitTicks,NULL);
}

// External Functions ---------------------------------------------------------
// Initialize the system
bool sysInit()
{
	// Initialize the system --------------------------------------------------
	// Setup the internal CPU clock source
	cpuSetOSCHF(CPU_SPEED,false,0);

	// Fill the stack area with pattern to detect max stack size
	memStackFill();

	// Initialize the system tick counter
	sysInitTick((TCB_t *)SYS_TICK_TIMER, SYS_TICK_FREQ);

	// Initialize the fsm scheduler
	fsmInit();
	
	uint32_t memProgRom = memProgramRomSize();
	uint16_t memProgRomUsed = memTextSize();
	uint16_t memConstRom = memConstRomSize();
	uint16_t memConstRomUsed = memRodataSize()+memOsTableSize();
	uint16_t memRam = memRamSize();
	uint16_t memRamUsed = memDataSize()+memHeapSize();
	INFO("CPU clock %d MHz",cpuGetFrequency()/1000);	
	INFO("Start sys tick %u Hz",SYS_TICK_FREQ);
	INFO("Program ROM used: %2d.%02d%%",percentWhole(memProgRomUsed,memProgRom),percentPlaces(memProgRomUsed,memProgRom));
	INFO("Const ROM used: %2d.%02d%%",percentWhole(memConstRomUsed,memConstRom),percentPlaces(memConstRomUsed,memConstRom));
	INFO("RAM used: %2d.%02d%%",percentWhole(memRamUsed,memRam),percentPlaces(memRamUsed,memRam));

	return(true);
}

// Set the system tick frequeny
void sysSetTickFreq(uint16_t sysTickFreq)
{
	uint32_t		cpuFreq = cpuGetFrequency();
	uint16_t		tickDivisor;

	sysTickFreq = sysTickFreq/1000;

	if(cpuFreq==1000)
		tickDivisor = 1000/sysTickFreq;
	else
		tickDivisor = cpuFreq/(2*sysTickFreq);

	// Disable interrupts while setting up timer registers
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// Set the top to divisor for tick freq
		((TCB_t *)SYS_TICK_TIMER)->CCMP = tickDivisor;
	}
}

// Return the system tick frequency in Hz
uint16_t sysGetTickFreq()
{
	uint32_t		cpuFreq = cpuGetFrequency();
	uint16_t		sysTickFreq;

	if(cpuFreq==1000)
		sysTickFreq = 1000/((TCB_t *)SYS_TICK_TIMER)->CCMP;
	else
		sysTickFreq = cpuFreq/(2*((TCB_t *)SYS_TICK_TIMER)->CCMP);

	return(sysTickFreq);
}

// Return the current system tick count
uint32_t sysGetTickCount()
{
	return(sysTicks);
}

// Put the system to sleep until the next interrupt
void sysSleep()
{
	set_sleep_mode(SLEEP_MODE_IDLE);
	sleep_mode();
	return;
}