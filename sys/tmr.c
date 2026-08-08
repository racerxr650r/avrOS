/**
 * @file tmr.c
 * @brief Precision Timer implementation — 32-bit RTC+TCB cascade scheduler.
 *
 * Implements the precision timer subsystem described in doc/SDD.md sec. 4.5.
 *
 * ### Hardware cascade
 *
 * The RTC free-runs as a 16-bit counter (PER = 0xFFFF) clocked at ~1024 Hz by
 * default. Its overflow event is routed through the peripheral event system
 * (EVSYS) to a TCB configured to count events (CLKSEL = EVENT, CNTMODE = INT).
 * The TCB therefore counts RTC overflows and forms the high 16 bits of a 32-bit
 * tick counter; the RTC.CNT is the low 16 bits.
 *
 * ### Software model
 *
 * Active timers are kept on a list ordered by absolute expiry time measured on
 * a module-local software tick clock (@ref tmrClock). @ref tmrClock is the tick
 * value at the instant the current head timer was armed; the ticks elapsed
 * since then are read from the live cascade (@ref tmrElapsed). Storing absolute
 * expiries means only @ref tmrClock advances (the SDD "deduct" step); the
 * individual timers need no per-tick bookkeeping. All comparisons are
 * wrap-safe, so the 32-bit clock may roll over freely.
 *
 * ### Two-stage compare interrupt
 *
 * To wake the head timer after a 32-bit delay D, D is split into a high part
 * (Dh overflows) and a low part (Dl ticks). Exactly one comparator interrupt is
 * enabled at a time (SDD sec. 4.5.4):
 *   1. Coarse: the TCB CAPT interrupt fires after Dh RTC overflows.
 *   2. Fine:   the RTC CMP interrupt fires after the remaining Dl ticks.
 * When a timer expires its event is signaled, the clock advances, and the next
 * head is re-armed.
 *
 * Created: 7/3/2026
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

// Externs --------------------------------------------------------------------
extern void *__start_TMR_TABLE,*__stop_TMR_TABLE;

// Hardware selection ---------------------------------------------------------
// Guard against sharing the TCB instance with the system tick.
#if TMR_TCB==SYS_TICK_TIMER
#error "TMR_TCB must differ from SYS_TICK_TIMER (the precision timer needs its own TCB)"
#endif

// Map the configured TCB base address to its pointer, interrupt vector, and
// EVSYS capture-user register.
#define TMR_TCB_PTR	((TCB_t *)TMR_TCB)
#if TMR_TCB==SYS_TIMER_TCB0
#define TMR_TCB_VECT	TCB0_INT_vect
#define TMR_TCB_USER	(&EVSYS.USERTCB0CAPT)
#elif TMR_TCB==SYS_TIMER_TCB1
#define TMR_TCB_VECT	TCB1_INT_vect
#define TMR_TCB_USER	(&EVSYS.USERTCB1CAPT)
#elif TMR_TCB==SYS_TIMER_TCB2
#define TMR_TCB_VECT	TCB2_INT_vect
#define TMR_TCB_USER	(&EVSYS.USERTCB2CAPT)
#else
#error "TMR_TCB must be one of SYS_TIMER_TCB0, SYS_TIMER_TCB1, SYS_TIMER_TCB2"
#endif

// Map the configured EVSYS channel to the channel-specific RTC overflow
// generator group code.
#if TMR_EVT_CHANNEL==EVT_CHANNEL0
#define TMR_EVT_GEN	EVSYS_CHANNEL0_RTC_OVF_gc
#elif TMR_EVT_CHANNEL==EVT_CHANNEL1
#define TMR_EVT_GEN	EVSYS_CHANNEL1_RTC_OVF_gc
#elif TMR_EVT_CHANNEL==EVT_CHANNEL2
#define TMR_EVT_GEN	EVSYS_CHANNEL2_RTC_OVF_gc
#elif TMR_EVT_CHANNEL==EVT_CHANNEL3
#define TMR_EVT_GEN	EVSYS_CHANNEL3_RTC_OVF_gc
#elif TMR_EVT_CHANNEL==EVT_CHANNEL4
#define TMR_EVT_GEN	EVSYS_CHANNEL4_RTC_OVF_gc
#elif TMR_EVT_CHANNEL==EVT_CHANNEL5
#define TMR_EVT_GEN	EVSYS_CHANNEL5_RTC_OVF_gc
#elif TMR_EVT_CHANNEL==EVT_CHANNEL6
#define TMR_EVT_GEN	EVSYS_CHANNEL6_RTC_OVF_gc
#elif TMR_EVT_CHANNEL==EVT_CHANNEL7
#define TMR_EVT_GEN	EVSYS_CHANNEL7_RTC_OVF_gc
#else
#error "TMR_EVT_CHANNEL must be one of EVT_CHANNEL0..EVT_CHANNEL7"
#endif

// Constants ------------------------------------------------------------------
#define TMR_TICKS_PER_OVF	65536UL	///< RTC ticks per overflow (16-bit range).

/** @brief Which comparator interrupt is currently armed for the head timer. */
typedef enum
{
	TMR_STAGE_IDLE = 0,	///< No active timer; both comparators disabled.
	TMR_STAGE_COARSE,	///< TCB CAPT interrupt armed (high-word overflows).
	TMR_STAGE_FINE		///< RTC CMP interrupt armed (low-word remainder).
} tmrStage_t;

// Locals ---------------------------------------------------------------------
static volatile timer_t		*tmrActiveHead = NULL;	///< Active timers, soonest first.
static volatile uint32_t	tmrClock = 0;			///< Tick clock at the head's arm time.
static volatile uint16_t	tmrArmR0 = 0;			///< RTC.CNT captured when head armed.
static volatile uint16_t	tmrArmN = 0;			///< Coarse overflow count programmed.
static volatile uint16_t	tmrArmLow = 0;			///< Fine-stage RTC compare target.
static volatile tmrStage_t	tmrStage = TMR_STAGE_IDLE;	///< Current comparator stage.

// Internal Function Prototypes -----------------------------------------------
static uint32_t tmrElapsed(void);
static void tmrArmHead(void);
static void tmrExpireHead(void);

// Command line interface -----------------------------------------------------
#ifdef TMR_CLI
ADD_COMMAND("tmr",tmrCmd,true);
static osStatus_t tmrCmd(int argc, char *argv[])
{
	// Walk the table of timers
	timerDescr_t *descr = (timerDescr_t *)&__start_TMR_TABLE;
	for(; descr < (timerDescr_t *)&__stop_TMR_TABLE; ++descr)
	{
		if(argc<2 || (argc==2 && !strcmp(descr->name,argv[1])))
		{
			printf(BOLD UNDERLINE FG_BLUE "%-20s %s\n\r" RESET,descr->name,descr->timer->active?"active":"idle");
			printf("\tDuration:%10lu\tRemaining:%10lu\n\r",descr->timer->duration,tmrGetRemaining(descr->timer));
		}
	}
	return(OS_OK);
}
#endif // TMR_CLI

// Internal Functions ---------------------------------------------------------
// Wrap-safe "is a before b" on the 32-bit tick clock.
static inline bool tmrBefore(uint32_t a, uint32_t b)
{
	return((int32_t)(a - b) < 0);
}

// Return the ticks elapsed since the current head timer was armed, read from
// the live RTC+TCB cascade. Must be called with interrupts disabled.
static uint32_t tmrElapsed(void)
{
	uint32_t elapsed = 0;

	if(tmrStage == TMR_STAGE_COARSE)
	{
		// elapsed = overflows*65536 + RTC.CNT - R0. Re-read the TCB overflow
		// count if a wrap lands between the two reads so RTC.CNT and the
		// overflow count stay consistent.
		uint16_t ovf = tcbGetCount(TMR_TCB_PTR);
		uint16_t cnt = rtcGetCount();
		if(tcbGetCount(TMR_TCB_PTR) != ovf)
		{
			ovf = tcbGetCount(TMR_TCB_PTR);
			cnt = rtcGetCount();
		}
		elapsed = ((uint32_t)ovf << 16) + cnt - tmrArmR0;
	}
	else if(tmrStage == TMR_STAGE_FINE)
	{
		// Coarse stage complete: tmrArmN overflows already elapsed and RTC has
		// wrapped to ~0, now climbing toward the low target.
		uint16_t cnt = rtcGetCount();
		elapsed = ((uint32_t)tmrArmN << 16) - tmrArmR0 + cnt;
	}

	return(elapsed);
}

// Insert a timer into the active list in expiry order. Returns true if it
// became the new head. Interrupts must be disabled.
static bool tmrListInsert(volatile timer_t *tmr)
{
	tmr->active = true;

	if(tmrActiveHead == NULL || tmrBefore(tmr->expiry, tmrActiveHead->expiry))
	{
		tmr->next = tmrActiveHead;
		tmrActiveHead = tmr;
		return(true);
	}

	volatile timer_t *cur = tmrActiveHead;
	while(cur->next != NULL && !tmrBefore(tmr->expiry, cur->next->expiry))
		cur = cur->next;
	tmr->next = cur->next;
	cur->next = tmr;
	return(false);
}

// Remove a timer from the active list. Interrupts must be disabled.
static void tmrListRemove(volatile timer_t *tmr)
{
	if(!tmr->active)
		return;

	if(tmrActiveHead == tmr)
	{
		tmrActiveHead = tmr->next;
	}
	else
	{
		volatile timer_t *cur = tmrActiveHead;
		while(cur != NULL && cur->next != tmr)
			cur = cur->next;
		if(cur != NULL)
			cur->next = tmr->next;
	}

	tmr->active = false;
	tmr->next = NULL;
}

// Program the two-stage compare interrupt for the current head timer. Any
// timers already due are expired first (chaining their events). Interrupts
// must be disabled.
static void tmrArmHead(void)
{
	while(tmrActiveHead != NULL)
	{
		uint32_t delta = tmrActiveHead->expiry - tmrClock;

		// Already due (equal or past expiry): expire immediately and continue.
		if((int32_t)delta <= 0)
		{
			tmrExpireHead();
			continue;
		}

		// Split the 32-bit delta into overflow count and low remainder,
		// offset by the current RTC position.
		uint16_t r0 = rtcGetCount();
		uint32_t sum = (uint32_t)r0 + delta;
		uint16_t n = (uint16_t)(sum >> 16);
		uint16_t low = (uint16_t)(sum & 0xFFFF);

		tmrArmR0 = r0;
		tmrArmN = n;
		tmrArmLow = low;

		if(n > 0)
		{
			// Coarse stage: count n RTC overflows on the TCB, then CAPT.
			rtcDisableInterrupt(RTC_INT_CMP);
			tcbDisableInterrupt(TMR_TCB_PTR, TCB_INT_CAPT);
			tcbSetCount(TMR_TCB_PTR, 0);
			tcbSetCompare(TMR_TCB_PTR, n);
			tcbClearInterruptFlags(TMR_TCB_PTR, TCB_INT_CAPT);
			tcbEnableInterrupt(TMR_TCB_PTR, TCB_INT_CAPT);
			tmrStage = TMR_STAGE_COARSE;
		}
		else
		{
			// Fine stage only: RTC compare after low ticks.
			tcbDisableInterrupt(TMR_TCB_PTR, TCB_INT_CAPT);
			rtcSetCompare(low);
			rtcClearInterruptFlags(RTC_INT_CMP);
			rtcEnableInterrupt(RTC_INT_CMP);
			tmrStage = TMR_STAGE_FINE;
		}
		return;
	}

	// No active timers remain: idle both comparators.
	tcbDisableInterrupt(TMR_TCB_PTR, TCB_INT_CAPT);
	rtcDisableInterrupt(RTC_INT_CMP);
	tmrStage = TMR_STAGE_IDLE;
}

// Expire the head timer: advance the clock, signal its event, and continue to
// the next timer. Interrupts must be disabled (called from ISR or arm path).
static void tmrExpireHead(void)
{
	volatile timer_t *head = tmrActiveHead;

	if(head == NULL)
	{
		tmrStage = TMR_STAGE_IDLE;
		return;
	}

	// Advance the module clock to this timer's expiry (the SDD "deduct" step:
	// every remaining timer's absolute expiry is now relative to the new base).
	tmrClock = head->expiry;
	tmrActiveHead = head->next;
	head->active = false;
	head->next = NULL;

	// Signal the associated event. For an ADD_TMR_ISR timer, evntTrigger runs
	// the user callback here in ISR context (regardless of arm state). For an
	// ADD_TMR timer with an FSM waiting (via tmrWait), the event is armed and
	// evntTrigger queues it so evntDispatch wakes the FSM. For an ADD_TMR timer
	// with a dispatch callback but no FSM waiting, arm it as a system event
	// first so its handler still runs from evntDispatch (self-arming pattern,
	// as the system tick does).
	volatile event_t *event = head->descr->event;
	if(event->descr->isrHandler == NULL &&
	   evntGetStatus(event) != EVENT_ARMED)
		evntArmSystem(event);
	evntTrigger(event, TMR_EVENT_EXPIRED);
}

// Interrupt Handlers ---------------------------------------------------------
// Coarse stage complete: the programmed number of RTC overflows have occurred.
// Hand off to the fine (RTC compare) stage, or expire if there is no remainder.
ISR(TMR_TCB_VECT)
{
	tcbClearInterruptFlags(TMR_TCB_PTR, TCB_INT_CAPT);
	tcbDisableInterrupt(TMR_TCB_PTR, TCB_INT_CAPT);

	if(tmrArmLow == 0)
	{
		// Expiry lands exactly on the overflow boundary; no fine stage needed.
		tmrExpireHead();
		tmrArmHead();
	}
	else
	{
		tmrStage = TMR_STAGE_FINE;
		rtcSetCompare(tmrArmLow);
		rtcClearInterruptFlags(RTC_INT_CMP);
		rtcEnableInterrupt(RTC_INT_CMP);

		// If the RTC has already passed the low target (tiny remainder vs ISR
		// latency), expire now rather than waiting a full 16-bit wrap.
		if(rtcGetCount() >= tmrArmLow)
		{
			rtcDisableInterrupt(RTC_INT_CMP);
			tmrExpireHead();
			tmrArmHead();
		}
	}
}

// Fine stage complete: the RTC compare matched. Expire the head timer and arm
// the next. (RTC_CNT_vect is shared with overflow, but only the compare
// interrupt is ever enabled here.)
ISR(RTC_CNT_vect)
{
	rtcClearInterruptFlags(RTC_INT_CMP);
	rtcDisableInterrupt(RTC_INT_CMP);
	tmrExpireHead();
	tmrArmHead();
}

// External Functions ---------------------------------------------------------
// Initialize the RTC+TCB+EVSYS cascade. Registered as an FSM initializer, so it
// runs from fsmInit() during system start-up.
ADD_INITIALIZER(tmr,tmrInit);
int tmrInit(const fsmStateMachineDescr_t *stateMachineDescr)
{
	UNUSED(stateMachineDescr);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// RTC: free-running 16-bit counter at the configured tick rate. Its
		// overflow drives the cascade; the compare drives the fine stage.
		rtcDisable();
		rtcSetClock(TMR_RTC_CLK);
		rtcSetPrescaler(TMR_RTC_PRESCALER);
		rtcSetPeriod(0xFFFF);
		rtcSetCount(0);
		rtcDisableInterrupt(RTC_INT_ALL);
		rtcClearInterruptFlags(RTC_INT_ALL);
		rtcEnable();

		// EVSYS: route RTC overflow to the cascade TCB's capture input.
		evtSetChannelGenerator(TMR_EVT_CHANNEL, TMR_EVT_GEN);
		evtSetUser(TMR_TCB_USER, TMR_EVT_CHANNEL);

		// TCB: count RTC overflow events; CAPT at CCMP forms the high 16 bits.
		tcbDisableInterrupt(TMR_TCB_PTR, TCB_INT_ALL);
		tcbClearInterruptFlags(TMR_TCB_PTR, TCB_INT_ALL);
		tcbSetClock(TMR_TCB_PTR, TCB_CLKSEL_EVENT_gc);
		tcbEventInputEnable(TMR_TCB_PTR, true);
		tcbSetMode(TMR_TCB_PTR, TCB_CNTMODE_INT_gc);
		tcbSetCount(TMR_TCB_PTR, 0);
		tcbEnable(TMR_TCB_PTR);

		tmrActiveHead = NULL;
		tmrClock = 0;
		tmrStage = TMR_STAGE_IDLE;
	}

	return(0);
}

// Arm or re-arm a timer.
bool tmrSetTimer(volatile timer_t *tmr, uint32_t ticks)
{
	// Reject zero and durations at/above 2^31: the wrap-safe expiry comparison
	// keeps deltas correct only up to just under half the 32-bit range
	// (~24 days at 1024 ticks/sec).
	if(tmr == NULL || ticks == 0 || ticks >= 0x80000000UL)
		return(false);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		uint32_t now = tmrClock + tmrElapsed();
		bool wasHead = (tmrActiveHead == tmr);

		if(tmr->active)
			tmrListRemove(tmr);

		tmr->duration = ticks;
		tmr->expiry = now + ticks;

		bool becomesHead = tmrListInsert(tmr);

		// Re-program the hardware if the head changed — either this timer
		// became the soonest, or it used to be the head being counted.
		if(becomesHead || wasHead)
		{
			tmrClock = now;
			tmrArmHead();
		}
	}

	return(true);
}

// Return the ticks remaining before a timer expires.
uint32_t tmrGetRemaining(volatile timer_t *tmr)
{
	uint32_t remaining = 0;

	if(tmr == NULL)
		return(0);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if(tmr->active)
		{
			uint32_t now = tmrClock + tmrElapsed();
			if(tmrBefore(now, tmr->expiry))
				remaining = tmr->expiry - now;
		}
	}

	return(remaining);
}

// Cancel a running timer.
bool tmrCancelTimer(volatile timer_t *tmr)
{
	if(tmr == NULL)
		return(false);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if(tmr->active)
		{
			uint32_t now = tmrClock + tmrElapsed();
			bool wasHead = (tmrActiveHead == tmr);

			tmrListRemove(tmr);

			// Cancel any pending FSM wait on the timer's event.
			if(evntGetStatus(tmr->descr->event) == EVENT_ARMED)
				evntDisarm(tmr->descr->event);

			if(wasHead)
			{
				tmrClock = now;
				tmrArmHead();
			}
		}
	}

	return(true);
}
