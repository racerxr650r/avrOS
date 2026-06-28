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
static volatile uint32_t	sysTicksPending = 0;

// Internal Function Prototypes ----------------------------------------------
static osStatus_t sysUpdateWaitTicks(volatile event_t *event);

// Event for system timer ticks to update the waiting state machines.
// Self-arming: the handler re-arms the event at the end of each tick.
ADD_EVENT(tick, sysUpdateWaitTicks);

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
	tcbClearInterruptFlags((TCB_t *)SYS_TICK_TIMER, TCB_INT_CAPT);
	
	// Increment the tick counter and the pending counter
	++sysTicks;
	++sysTicksPending;

	// Trigger the timer update event (may fail if previous tick not yet dispatched)
	evntTrigger(&tick,EVENT_TYPE_TICK);	
}

// Command line interface -----------------------------------------------------
#ifdef SYS_CLI
ADD_COMMAND("tick",tickCmd,true);
ADD_COMMAND("tickFreq",tickFreqCmd);
#endif

osStatus_t tickCmd(int argc, char *argv[])
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

	return(OS_OK);
}

osStatus_t tickFreqCmd(int argc, char *argv[])
{
	osStatus_t ret = OS_INVALID;

	if(argc == 2)
	{
		uint16_t freq = atoi(argv[1]);

		if(freq >= 1000)
		{
			sysSetTickFreq(freq);
			ret = OS_OK;
		}
	}

	return(ret);
}

// Internal Functions ---------------------------------------------------------
// Tick event handler: wake any FSMs whose wait-tick counters have expired
// and re-arm the tick event for the next ISR trigger.
// Drains sysTicksPending so that ticks missed while the main loop was busy
// (e.g. during UART I/O) are applied before re-arming.
static osStatus_t sysUpdateWaitTicks(volatile event_t *event)
{
	uint32_t pending;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		pending = sysTicksPending;
		sysTicksPending = 0;
	}

	while(pending--)
		fsmUpdateWaitTicks();

	evntArmSystem(event);
	return(OS_OK);
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
		// Select the peripheral clock source (DIV1 or DIV2)
		tcbSetClock(tcb, clockSource);
		// Set the top to tick divisor for tick freq
		tcbSetCompare(tcb, tickDivisor);
		// Enable the capture/timeout interrupt
		tcbEnableInterrupt(tcb, TCB_INT_CAPT);
		// Setup periodic timer mode
		tcbSetMode(tcb, TCB_CNTMODE_INT_gc);
		// Enable the clock
		tcbEnable(tcb);
	}

	evntArmSystem(&tick);
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

	// Initialize the event manager (populates disarmed list from EVNT_TABLE)
	evntInit();

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
		tcbSetCompare((TCB_t *)SYS_TICK_TIMER, tickDivisor);
	}
}

// Return the system tick frequency in Hz
uint16_t sysGetTickFreq()
{
	uint32_t		cpuFreq = cpuGetFrequency();
	uint16_t		sysTickFreq;

	if(cpuFreq==1000)
		sysTickFreq = 1000/tcbGetCapture((TCB_t *)SYS_TICK_TIMER);
	else
		sysTickFreq = cpuFreq/(2*tcbGetCapture((TCB_t *)SYS_TICK_TIMER));

	return(sysTickFreq);
}

// Return the current system tick count
uint32_t sysGetTickCount()
{
	uint32_t ticks;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		ticks = sysTicks;
	}
	return(ticks);
}

// Put the system to sleep until the next interrupt
void sysSleep()
{
	slpSleep(SLPCTRL_SMODE_IDLE_gc);
	return;
}