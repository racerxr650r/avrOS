[![avrOS](./images/avrOS.gif "avrOS")](https://github.com/racerxr650r/avrOS)
---
## avrOS Software Design Document
**Project:** avrOS  
**Author:** John Anderson \<racerxr650r@gmail.com\>  
**Date:** February 24, 2026  
**Version:** 1.0  

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [System Overview](#2-system-overview)
3. [Architecture](#3-architecture)
4. [System Modules](#4-system-modules)
   - 4.1 [System Kernel (sys)](#41-system-kernel-sys)
   - 4.2 [Finite State Machine Manager (fsm)](#42-finite-state-machine-manager-fsm)
   - 4.3 [Event Manager (event)](#43-event-manager-event)
   - 4.4 [Queue Manager (queue)](#44-queue-manager-queue)
   - 4.5 [Precision Timer](#45-precision-timer)
   - 4.6 [File I/O Abstraction (fio)](#46-file-io-abstraction-fio)
5. [Services](#5-services)
   - 5.1 [Command Line Interface (cli)](#51-command-line-interface-cli)
   - 5.2 [Logging Service (log)](#52-logging-service-log)
   - 5.3 [PCM Audio Service (pcm)](#53-pcm-audio-service-pcm)
   - 5.4 [Unit Test Service (uts)](#54-unit-test-service-uts)
6. [Drivers](#6-drivers)
   - 6.1 [CPU Driver (cpu)](#61-cpu-driver-cpu)
   - 6.2 [Memory Driver (mem)](#62-memory-driver-mem)
   - 6.3 [UART Driver (uart)](#63-uart-driver-uart)
   - 6.4 [DAC Driver (dac)](#64-dac-driver-dac)
   - 6.5 [GPIO Driver (gpio)](#65-gpio-driver-gpio)
   - 6.6 [Inline Register Drivers — Overview](#66-inline-register-drivers--overview)
   - 6.7 [Clock Driver (clk)](#67-clock-driver-clk)
   - 6.8 [Sleep Driver (slp)](#68-sleep-driver-slp)
   - 6.9 [Reset Driver (rst)](#69-reset-driver-rst)
   - 6.10 [Watchdog Driver (wdt)](#610-watchdog-driver-wdt)
   - 6.11 [Non-Volatile Memory Driver (nvm)](#611-non-volatile-memory-driver-nvm)
   - 6.12 [Interrupt Controller Driver (int)](#612-interrupt-controller-driver-int)
   - 6.13 [Timer/Counter Type A Driver (tca)](#613-timercounter-type-a-driver-tca)
   - 6.14 [Timer/Counter Type B Driver (tcb)](#614-timercounter-type-b-driver-tcb)
   - 6.15 [Real-Time Counter Driver (rtc)](#615-real-time-counter-driver-rtc)
   - 6.16 [Event System Driver (evt)](#616-event-system-driver-evt)
   - 6.17 [Voltage Reference Driver (vref)](#617-voltage-reference-driver-vref)
   - 6.18 [ADC Driver (adc)](#618-adc-driver-adc)
   - 6.19 [Analog Comparator Driver (ac)](#619-analog-comparator-driver-ac)
   - 6.20 [Zero-Cross Detector Driver (zcd)](#620-zero-cross-detector-driver-zcd)
   - 6.21 [SPI Driver (spi)](#621-spi-driver-spi)
   - 6.22 [TWI Driver (twi)](#622-twi-driver-twi)
   - 6.23 [Port Multiplexer Driver (pmux)](#623-port-multiplexer-driver-pmux)
   - 6.24 [I/O Port Driver (pio)](#624-io-port-driver-pio)
7. [Configuration](#7-configuration)
8. [Application Interface](#8-application-interface)
9. [Verification and Testing](#9-verification-and-testing)
10. [Revision History](#10-revision-history)

---

## 1. Introduction

### 1.1 Purpose

This Software Design Document (SDD) describes the design and implementation of avrOS, a lightweight, cooperative operating system for Microchip AVR-Dx series microcontrollers. It provides the software architecture, module descriptions, interfaces, and data structures used throughout the system.

### 1.2 Scope

avrOS provides a finite-state-machine-based cooperative scheduler, an event system, queue-based inter-task communication, peripheral drivers, and application services. It is intended for resource-constrained embedded systems where deterministic behavior and minimal memory overhead are required.

### 1.3 Definitions and Acronyms

| Term | Definition |
|------|-----------|
| FSM  | Finite State Machine |
| ISR  | Interrupt Service Routine |
| TCB  | Timer/Counter type B (AVR peripheral) |
| UART / USART | Universal Asynchronous/Synchronous Receiver-Transmitter |
| DAC  | Digital-to-Analog Converter |
| GPIO | General Purpose Input/Output |
| CLI  | Command Line Interface |
| PCM  | Pulse-Code Modulation |
| RAM  | Random Access Memory |
| ROM  | Read-Only Memory (Flash) |
| FIO  | File I/O |
| TMR  | Timer |
| UTS  | Unit Test Service |

### 1.4 References

- Microchip AVR-Dx Family Datasheet
- AVR-libc Documentation
- [doc/MANUAL.md](MANUAL.md)
- [avrOS.h](../avrOS.h)

---

## 2. System Overview

avrOS is a cooperative, event-driven operating system built around a finite state machine (FSM) dispatcher. All tasks in the system are implemented as FSMs registered at compile time using linker-section macros. The scheduler dispatches each FSM in priority order on every scan cycle. Drivers and services register initialization functions that are called once at startup by `sysInit()`.

The OS makes no use of dynamic memory allocation after initialization. All data structures — FSM descriptors, UART instances, GPIO instances, CLI commands, and queue buffers — are placed in dedicated linker sections in flash (ROM) at compile time.

At runtime, the kernel follows a deterministic scan-loop model:

1. Wake from idle on an asynchronous event represented by an interrupt.
2. Dispatch each registered FSM once according to priority.
3. Drain pending events and apply state-transition requests.
4. Return to idle sleep until the next interrupt source occurs.

Event dispatch is performed at least once at the start of each scan cycle and rechecked during FSM traversal if new events are triggered. This does not reprocess the same queued event; each queued event is consumed once per enqueue, but additional events raised mid-scan can be handled in the same cycle.

This model keeps execution predictable and avoids preemption-related race conditions in application code. State handlers are expected to be short, non-blocking functions that defer long operations across multiple scans.

Module discovery and composition are link-time driven. Subsystems register descriptors through `ADD_STATE_MACHINE`, `ADD_INITIALIZER`, and similar macros that place records into named sections. During startup, `sysInit()` and the FSM manager iterate these tables to assemble the running system without manual registry code in `main()`.

Because objects are statically allocated, RAM usage is fixed and analyzable before deployment. The design targets resource-constrained AVR-Dx devices where bounded memory, small code size, and repeatable timing are primary requirements. Combined with a single-threaded cooperative scheduler, this supports straightforward worst-case execution analysis and simplifies debugging on hardware.

The architecture separates concerns into four layers:

- Drivers (`drv/`) provide hardware abstraction for CPU, UART, GPIO, DAC, and memory.
- System modules (`sys/`) provide core OS services such as eventing, queues, FSM dispatch, and tick management.
- Services (`srv/`) provide reusable features (CLI, logging, PCM) built on the system layer.
- Applications (`app/`) define product-specific behavior by composing FSMs and service interfaces.

In normal operation, timing is anchored by the system tick ISR, while work execution remains in foreground cooperative context. This yields low interrupt complexity: ISRs signal events or update counters, and functional processing is performed by FSM handlers in the main loop.

---

## 3. Architecture

### 3.1 Layered Architecture

```
+-----------------------------------------------------------+
|                  Application (app/)                       |
+-----------------------------------------------------------+
|          Services (srv/): CLI, Log, PCM                   |
+-----------------------------------------------------------+
|   System (sys/): FSM, Event, Queue, FIO, Sys              |
+-----------------------------------------------------------+
|      Drivers (drv/): CPU, MEM, UART, DAC, GPIO            |
+-----------------------------------------------------------+
|              AVR-Dx Hardware / AVR-libc                   |
+-----------------------------------------------------------+
```

### 3.2 Scheduling Model

- Cooperative, round-robin dispatch within priority bands.
- Four priority levels: `FSM_DRV` (highest), `FSM_SYS`, `FSM_SRV`, `FSM_APP` (lowest).
- Each FSM state handler in the ready queue is called once per scan cycle; it must return without blocking.
- State transitions are requested via `fsmSetNextState()` and take effect on the next dispatch.
- State machines can be moved to the wait queue when they request to wait on one or more events
- State machines are then returned to the ready queue when an event they are waiting on is triggered

### 3.3 Startup Sequence

1. `main()` calls `sysInit()`.
2. `sysInit()` iterates the `FSM_TABLE` linker section, calling all registered initializer functions.
3. Peripheral drivers (UART, GPIO, etc.) initialize hardware from their `ADD_INITIALIZER` entries.
4. `main()` enters the main loop, repeatedly calling `fsmDispatch()`.
5. Between dispatch calls, `sysSleep()` places the CPU into idle sleep until the next tick ISR wakes it.

---

### 3.4 Memory Layout

The reference target is the AVR128DA28 (96 KiB flash, 16 KiB SRAM). The
toolchain's default AVR-Dx linker script owns the base memory map and
startup symbols (including FLMAP setup used by newer avr-gcc/avr-libc).
avrOS extends that layout with
[app/avrOS_example/avrOS-sections.x](../app/avrOS_example/avrOS-sections.x),
an additive linker fragment that runs via `INSERT AFTER .rodata;`.
Read-only data and OS descriptor tables live in the mapped flash window
so they can be iterated through ordinary C pointers without
`pgm_read_*()` calls.

| Region | Address | Length | Contents |
|--------|---------|--------|----------|
| Program flash (unmapped) | `0x000000`–`0x017FFF` | 96 KiB | `.text`, vectors, C runtime |
| Flash window (mapped to data) | `0x018000`–`0x01FFFF` | 32 KiB | `.rodata`, `CLI_CMDS`, `FSM_TABLE`, `QUE_TABLE`, `TMR_TABLE`, `EVNT_TABLE`, `GPIO_TABLE`, `UART_TABLE` |
| SRAM | `0x004000`–`0x0143FF` | 16 KiB | `.data`, `.bss`, `.noinit`, stack |
| EEPROM | `0x010000`–`0x01FFFF` | 64 KiB | `.eeprom` (unused today) |
| Fuses / lock / signatures | `0x020000`+ | — | Programmed by `make fuses` / `make lock_bits` |

The `drv/mem` driver exposes helpers that surface each region at
runtime; the `ram` and `rom` CLI commands print the values:

| Symbol (linker) | Helper | Returns |
|-----------------|--------|---------|
| `_etext` | `memTextSize()` | Bytes used by `.text` |
| `__start_text_window` / `__stop_text_window` | `memConstSize()` | Total mapped const region |
| `__start_text_window` / `__stop_rodata` | `memRodataSize()` | `.rodata` portion |
| `__stop_rodata` / `__stop_text_window` | `memOsTableSize()` | OS table portion |
| `PROGMEM_SIZE - MAPPED_PROGMEM_SIZE` | `memProgramRomSize()` | Unmapped flash budget |
| `MAPPED_PROGMEM_SIZE` | `memConstRomSize()` | Mapped flash budget |
| `__data_start` / `__data_end` | `memDataSize()` | `.data` (initialized RAM) |
| `__heap_start` / `__brkval` | `memHeapSize()` | Heap (never grown) |
| `RAMEND - SP` | `memStackSize()` / `memStackSizeMax()` | Current / high-water stack |
| `RAMSIZE` | `memRamSize()` | Total SRAM |

`memStackFill()` writes a `0xDEADBEEF` byte pattern from `__heap_start`
up to the current SP at boot. `memStackSizeMax()` walks down from
`RAMEND` looking for that pattern to compute the high-water mark.

### 3.5 Linker Sections and Descriptor Tables

Each `ADD_<THING>` macro places a `const` descriptor into a named
section in the flash window. The additive linker fragment
([app/avrOS_example/avrOS-sections.x](../app/avrOS_example/avrOS-sections.x))
publishes `__start_<NAME>` / `__stop_<NAME>` symbols around each
section so the OS can iterate them at runtime:

| Section | Producer macro | Iterated by |
|---------|----------------|-------------|
| `CLI_CMDS`    | `ADD_COMMAND`        | `srv/cli.c` |
| `FSM_TABLE`   | `ADD_STATE_MACHINE`, `ADD_INITIALIZER` | `sys/fsm.c`, `sys/sys.c` |
| `QUE_TABLE`   | `ADD_QUEUE`          | `sys/queue.c` |
| `TMR_TABLE`   | `ADD_TMR`, `ADD_TMR_ISR` | `sys/tmr.c` |
| `TEST_TABLE`  | `ADD_TEST`           | `srv/uts.c` |
| `EVNT_TABLE`  | `ADD_EVENT`          | `sys/event.c` |
| `GPIO_TABLE`  | `ADD_GPIO`           | `drv/gpio.c` |
| `UART_TABLE`  | `ADD_UART_RW`, `ADD_UART_WO`, `ADD_UART_RO` | `drv/uart.c` |

`SECTION(x)` is `__attribute__((__used__, __section__(#x)))`. The
`__used__` is required — otherwise the linker may discard a descriptor
that has no direct C reference.

To **add a new descriptor table**:

1. Pick a name `<MOD>_TABLE` (uppercase, ends `_TABLE`).
2. Add a block to
   [app/avrOS_example/avrOS-sections.x](../app/avrOS_example/avrOS-sections.x)
   before the `__stop_text_window` assignment. Use `KEEP(*(YOUR_TABLE))`
   in the block so descriptors are not removed by `--gc-sections`.
3. In C, mark the descriptor with `SECTION(<MOD>_TABLE)` and declare
   `extern void *__start_<MOD>_TABLE, *__stop_<MOD>_TABLE;` in the
   module that iterates it.

Notes:

- Keep using the toolchain's default AVR-Dx linker script for base
  sections, startup, and FLMAP runtime symbols.
- Use the avrOS fragment only for descriptor-table placement and linker
  boundary symbols consumed by `drv/mem` and table iterators.

Rules:

- Runtime mutable data is **never** placed in a `<MOD>_TABLE`. The
  flash window is read-only.
- Section names match exactly across the C source and the linker
  script — every module's CLI / init code depends on the precise
  `__start_*` / `__stop_*` symbol name.

---

## 3.6 Error Handling and Return Codes

### Overview

All avrOS subsystems use a unified, negative-valued error code enumeration `osStatus_t`
to report success/failure. This design coexists with functions that return counts or
state values by leveraging the invariant: **all errors are negative, all data is
non-negative**. Existing code that tests `if (ret < 0)` or `if (ret != 0)` continues to
work unchanged, allowing migration to happen incrementally.

### Return Code Semantics

**`osStatus_t` enumeration (defined in [avrOS.h](../avrOS.h)):**

| Code | Name | Meaning |
|------|------|---------|
| 0 | `OS_OK` | Success |
| -1 | `OS_ERROR` | Generic or unspecified failure |
| -2 | `OS_INVALID` | Invalid argument: NULL pointer, out-of-range value, bad enum |
| -3 | `OS_NOTFOUND` | Named object, handle, or device not found |
| -4 | `OS_STATE` | Operation not valid in the current state |
| -5 | `OS_BUSY` | Resource busy / would block (spinlock, full descriptor table) |
| -6 | `OS_EMPTY` | No data available (queue empty, buffer empty, no event) |
| -7 | `OS_FULL` | No space available (queue full, buffer full, descriptor table full) |
| -8 | `OS_TIMEOUT` | Operation timed out waiting for a condition |
| -9 | `OS_NORESOURCE` | Out of memory, out of handles, out of descriptors |
| -10 | `OS_IO` | Hardware error, peripheral I/O error, or I/O timeout |
| -11 | `OS_UNSUPPORTED` | Operation not implemented or unsupported |

### Function Return Patterns

**Pattern 1: Pure success/failure** — return `osStatus_t` directly.
```c
osStatus_t fsmReady(volatile fsmStateMachine_t *fsm) {
    if (fsm == NULL) return OS_INVALID;
    // ... transition to ready queue ...
    return OS_OK;
}
```

**Pattern 2: Count/value with error fallthrough** — return non-negative count on
success, negative `osStatus_t` on error.
```c
int uartTransmit(const uart_t *uart, const void *buf, size_t n) {
    if (uart == NULL || buf == NULL) return OS_INVALID;  // negative
    // ... write to FIFO ...
    return bytes_written;  // non-negative
}
```
Callers test with `if (ret < 0)` or `OS_FAILED(ret)`.

**Pattern 3: Domain-specific state enum** — some operations return a state enum
(e.g., `evntState_t`) to convey both state and error. These remain distinct from
`osStatus_t` but align error values: `-1` in `evntState_t` (EVENT_ERROR) corresponds
conceptually to `OS_ERROR`.

### Helper Macros

| Macro | Purpose |
|-------|---------|
| `OS_FAILED(s)` | True if `s < 0` |
| `OS_SUCCEEDED(s)` | True if `s >= 0` |

---

## 4. System Modules

### 4.1 System Kernel (sys)

**Files:** [sys/sys.c](../sys/sys.c), [sys/sys.h](../sys/sys.h)

#### 4.1.1 Responsibilities

- Initialize the system tick timer (TCB peripheral).
- Provide the system tick counter accessible to all modules.
- Provide the `sysSleep()` idle mechanism.

#### 4.1.2 Key Interfaces

| Function | Description |
|----------|-------------|
| `bool sysInit()` | Initialize the system; calls all registered initializers. |
| `void sysSetTickFreq(uint16_t hz)` | Set the system tick interrupt frequency. |
| `uint16_t sysGetTickFreq()` | Return the current tick frequency (kHz). |
| `uint32_t sysGetTickCount()` | Return the running tick counter value. |
| `void sysSleep()` | Sleep until next tick interrupt. |

#### 4.1.3 Configuration

| Macro | Description |
|-------|-------------|
| `SYS_TICK_TIMER` | Selects TCB0, TCB1, or TCB2 as the tick timer source. |

#### 4.1.4 Design Notes

- The tick ISR calls `evntTrigger(&tick, EVENT_TYPE_TICK)` to mark the `tick` event triggered. It also increments `sysTicksPending`, a counter of ticks not yet applied to the FSM wait counters.
- The `tick` event is registered with `ADD_EVENT(tick, sysUpdateWaitTicks)`, binding a custom handler. `evntDispatch()` invokes `sysUpdateWaitTicks()` in main-loop context, which drains `sysTicksPending` and decrements wait-tick counters across all FSMs (one `fsmUpdateWaitTicks()` call per pending tick) and re-arms the event via `evntArmSystem()`. This is the canonical example of the self-arming pattern (see §4.3.3).
- **Missed-tick recovery.** If the main loop is busy (e.g. blocked on UART I/O) when the tick ISR fires, the tick may not yet be dispatched and `evntTrigger` is a no-op for that cycle. The `sysTicksPending` counter ensures no tick is lost: the next `sysUpdateWaitTicks()` applies every accumulated tick before re-arming, keeping FSM timing accurate under load.
- `sysSleep()` uses the AVR `sleep_mode()` facility; the CPU wakes automatically on the next tick ISR.

---

### 4.2 Finite State Machine Manager (fsm)

**Files:** [sys/fsm.c](../sys/fsm.c), [sys/fsm.h](../sys/fsm.h)

#### 4.2.1 Responsibilities

- Maintain a table of all registered FSMs and initializers.
- Dispatch FSM state handlers in priority order each scan cycle.
- Manage state transitions and track current/previous/next state names.
- Call the evntDispatcher() at the start of each scan cycle and after each state is called
- Provide the `ADD_STATE_MACHINE` and `ADD_INITIALIZER` registration macros.

#### 4.2.2 Data Structures

| Structure | Description |
|-----------|-------------|
| `fsmStateMachine_t` | Runtime state of one FSM (RAM): current/prev/next state, tick count, linked-list pointer. |
| `fsmStateMachineDescr_t` | Compile-time descriptor (ROM): name, pointer to runtime state, handler, priority, instance data. |

#### 4.2.3 Priority Levels

The `priority` argument to `ADD_STATE_MACHINE` is a single byte that
encodes both a *class* (top 2 bits) and a *sub-priority* (low 6 bits).
Lower numeric values run first in the ready list.

```
 7 6 5 4 3 2 1 0
+---+-------------+
|cls| sub-priority|
+---+-------------+
  |        |
  |        +--- 0..63, larger = lower priority within the class
  +------------ 00=DRV, 01=SYS, 10=SRV, 11=APP
```

| Constant | Value | Description | Use for |
|----------|-------|-------------|---------|
| `FSM_DRV` | `0x00` | Driver priority (highest) | Hardware drivers — periodic polling, ISR back-half. |
| `FSM_SYS` | `0x40` | System priority | Kernel-level support (reserved — the dispatchers themselves are not state machines today). |
| `FSM_SRV` | `0x80` | Service priority | Higher-level OS services (`cli`, `log`, `btn`, `pcm`). |
| `FSM_APP` | `0xC0` | Application priority (lowest) | Application state machines. |

Pick the class that matches the directory the module lives in
(`drv/` → `FSM_DRV`, `srv/` → `FSM_SRV`, `app/` → `FSM_APP`).
Sub-priority conventions in the existing tree:

| Sub-priority | Use |
|--------------|-----|
| `0` – `15` | Critical / latency-sensitive (drain UART, button debounce). |
| `16` – `47` | Normal background work. |
| `48` – `63` | Catch-all / lowest within class. The CLI uses `FSM_SRV \| 0x3f` so operator commands never starve real work. |

Examples:

```c
// Application LED driver — happy to be preempted by anything.
ADD_STATE_MACHINE(Leds_sm, ledsInit, FSM_APP | 10);

// CLI — service class, lowest sub-priority so commands never starve drivers.
ADD_STATE_MACHINE(cli_SM, cliInit, FSM_SRV | 0x3f, &cliInstance);
```

Anti-patterns:

- Sub-priority `0` in an application FSM — claims higher priority than
  every driver in its class. Use `FSM_APP | 10` or higher.
- Multiple state machines tied at the same numeric priority — link
  order decides which runs first, which is fragile.
- Treating the byte as a bitmask — only the top 2 bits encode the
  class; the rest is plain numeric priority.

#### 4.2.4 Key Interfaces

| Function / Macro | Description |
|-----------------|-------------|
| `ADD_STATE_MACHINE(name, initFn, priority, ...)` | Register an FSM with an optional instance pointer. |
| `ADD_INITIALIZER(name, handler, ...)` | Register a one-shot initializer called at startup. |
| `fsmSetNextState(sm, state)` | Request a transition to the named state on the next cycle. |
| `bool fsmIsInitialCall()` | True on the first call to the current state after a transition. |
| `uint32_t fsmScanCycle()` | Return the total number of dispatch cycles executed. |
| `void* fsmGetInstance(sm)` | Retrieve the instance pointer associated with an FSM. |

#### 4.2.5 Design Notes

- All descriptors are placed in the `FSM_TABLE` linker section in flash; no runtime registration is needed.
- Initializers have a `NULL` `stateMachine` pointer to distinguish them from FSMs during startup.
- The `fsmIsInitialCall()` helper allows one-time entry actions within a state without requiring a separate sub-state.

---

### 4.3 Event Manager (event)

**Files:** [sys/event.c](../sys/event.c), [sys/event.h](../sys/event.h)

#### 4.3.1 Responsibilities

- Provide a mechanism for FSMs to wait on one or more asychornous hardware or software conditions.
- Allow ISRs and other FSMs to signal events that wake waiting state machines.
- Decouple ISR signaling from handler execution: ISRs only mark an event triggered; handlers run from `evntDispatch()` in main-loop context.
- When a FSM is waiting on more than one event and a single event triggers, all other events that FSM is waiting on are moved to the DISARM queue. The FSM will need to explicitly re-ARM any events it wished to wait on
- When a FSM ARMs an event, it is moved from the READY queue to the WAIT queue

#### 4.3.2 Key Interfaces

| Function / Macro | Description |
|------------------|-------------|
| `ADD_EVENT(name [, handler])` | Define an event object + flash descriptor; binds an optional dispatch-context handler (defaults to `evntHandler`). |
| `ADD_EVENT_ISR(name, isrHandler)` | Define an event object + flash descriptor bound to a direct **interrupt-context** handler (`evntIsrHandler_t`, returns `void`). Runs from `evntTrigger` in the producer's ISR, not from `evntDispatch`. |
| `evntArm(event)` | Move an event from the disarmed list to the armed list. The event's `stateMachine`/`fsmState` are set by the caller (typically `evntWait`). |
| `evntArmSystem(event)` | Arm a system-wide event with no associated state machine. Used by self-arming events (e.g. system tick) whose handler runs directly from `evntDispatch`. |
| `evntDisarm(event)` | Move an event from the armed list back to the disarmed list. |
| `evntWait(event, eventType, fsmState)` | Resolve the calling FSM (via `fsmGetCurrentStateMachine()`), record `eventType` in `event->type` and the resume handler in `event->fsmState`, arm the event, and place the FSM in the wait queue. The default handler wakes the FSM only when a producer triggers a matching sub-type. |
| `evntTrigger(event, triggerType)` | ISR-safe. If the event is armed, move it from armed to triggered and record `triggerType` in `event->triggerType`. Triggering an unarmed event is a no-op (normal for fire-and-forget queue events) and is not counted as an error. |
| `evntDispatch()` | Called by `fsmDispatch()`. Pops each triggered event, calls its handler, then (for FSM-bound events) moves it and any sister events sharing the same `stateMachine` to the disarmed list. |
| `evntGetStatus / evntGetType / evntGetTrigger / evntGetStateMachine` | Inline accessors for handler use. |
| `evntInit()` | Walks the `EVNT_TABLE` and populates the disarmed list. Called by `sysInit()`. |

#### 4.3.3 Design Notes

- **Three global lists.** The event manager owns three intrusive linked lists threaded through `event_t.next`: `evntListDisarmed`, `evntListArmed`, `evntListTriggered`. Every event is on exactly one list at any moment. Lifecycle: `disarmed → armed → triggered → disarmed`.
- **Descriptor / status split.** `evntDescriptor_t` lives in flash (`EVNT_TABLE`) and carries the event name, the dispatch-context `handler` pointer, and the interrupt-context `isrHandler` pointer (exactly one is set; the other is `NULL`). `event_t` is the RAM status object that carries list linkage, current state, the resume handler `fsmState`, and the `type`/`triggerType` sub-type pair. `ADD_EVENT(name)` emits both and links them.
- **Handler binding.** The handler pointer is in the descriptor (read-only) and selected at compile time by `ADD_EVENT`. The 1-arg form (`ADD_EVENT(name)`) binds the default `evntHandler`, which — when `type == triggerType` — sets the waiting FSM's next state to `event->fsmState` (via `fsmSetNextState`) and moves it to the ready queue (via `fsmReady`). The 2-arg form (`ADD_EVENT(name, fn)`) binds a custom handler with signature `int (*)(volatile event_t *)`.
- **ISR contract.** ISRs call only `evntTrigger(event, triggerType)`. For an event defined with `ADD_EVENT`, the handler runs from `evntDispatch()` in main-loop (cooperative) context, never in ISR context. For an event defined with `ADD_EVENT_ISR`, `evntTrigger` instead invokes the `evntIsrHandler_t` immediately in the caller's (interrupt) context and does not queue the event for dispatch — used by the precision timer for least-jitter expiry callbacks. Such a handler must be short and returns `void`.
- **Self-arming pattern.** A system event whose handler runs directly from `evntDispatch` (no FSM is waiting) uses `evntArmSystem` to re-arm itself at the end of the handler. `evntDispatch` recognizes `event->stateMachine == NULL` as a system event and skips its auto-disarm-and-sister-scan path so the re-arm is preserved. Reference implementation: `sysUpdateWaitTicks()` in [sys/sys.c](../sys/sys.c).
- **Wakeup semantics for FSM-bound events.** When an FSM-bound event's handler returns, `evntDispatch` also disarms any other events armed for the same `stateMachine`. This implements "wait on any one of N events" — only the first to trigger fires; the others are silently disarmed.

#### 4.3.4 Event Sub-Type Contract

`event_t` carries two sub-type fields (`uint8_t`) that together implement the
sub-type contract:

| Field | Set by | When |
|-------|--------|------|
| `type` | `evntWait(ev, type, fsmState)` | When a consumer arms the event for the condition it wants to wait for. |
| `triggerType` | `evntTrigger(ev, triggerType)` | When a producer raises the event (ISR or other state machine). |

The default handler `evntHandler()` releases the waiting state machine
only when `event->type == event->triggerType`. That lets one event
object multiplex several sub-conditions without spurious wakeups.

Sub-type registries:

- **System tick** ([sys/sys.h](../sys/sys.h)) — `EVENT_TYPE_TICK = 1`.
  Exactly one sub-type.
- **Queues** ([sys/queue.h](../sys/queue.h)) —
  `QUE_EVENT_EMPTY = 1`, `QUE_EVENT_NOT_EMPTY = 2`,
  `QUE_EVENT_FULL = 3`, `QUE_EVENT_NOT_FULL = 4`. Raised by `quePut` /
  `queGet` as the buffer crosses boundaries.
- **GPIO** ([drv/gpio.h](../drv/gpio.h)) — `gpioEventType_t`:
  `GPIO_EVENT_BOTHEDGES = 1`, `GPIO_EVENT_RISING = 2`,
  `GPIO_EVENT_FALLING = 3`, `GPIO_EVENT_LEVEL_LOW = 4`. The sub-type is
  declared per-GPIO in `ADD_GPIO(...)` and also configures the port
  pin's ISC (interrupt-sense control) at init time.

When introducing a new module that owns an event:

1. Define a `typedef enum { ... } <mod>Events_t;` in the module's
   header, starting at `1`. `0` is reserved as the "armed but not
   yet triggered" sentinel.
2. Document each value with a trailing `///<` comment.
3. Reserve a contiguous range; do not reuse numbers across modules
   that share an event object.

Rules:

- Never use `0` as a sub-type — it is the idle sentinel.
- Sub-types are per-event-object scope; two different events may both
  use sub-type `1` for different meanings.
- The consumer's `evntWait` sub-type must match exactly one of the
  producer's `evntTrigger` sub-types or the state machine will sit
  forever in the wait queue (use the `evnt` CLI command to diagnose).

Custom handlers — `ADD_EVENT(name, myHandler)` installs an alternate
handler. It runs from `evntDispatch()` between state-machine passes
(not in ISR context), but it executes before the next state-machine
handler — keep it short.

---

### 4.4 Queue Manager (queue)

**Files:** [sys/queue.c](../sys/queue.c), [sys/queue.h](../sys/queue.h)

#### 4.4.1 Responsibilities

- Provide fixed-size, statically-allocated circular byte queues.
- Signal events on enqueue/dequeue to wake waiting FSMs.
- Support interrupt-safe access via atomic operations.

#### 4.4.2 Key Interfaces

| Function / Macro | Description |
|-----------------|-------------|
| `ADD_QUEUE(name, itemSize, depth)` | Declare and statically allocate a named queue. |
| `bool queEnqueue(queue_t *q, void *item)` | Add an item; returns false if full. |
| `bool queDequeue(queue_t *q, void *item)` | Remove an item; returns false if empty. |
| `bool queIsEmpty(queue_t *q)` | True if the queue contains no items. |
| `bool queIsFull(queue_t *q)` | True if the queue is at capacity. |

#### 4.4.3 Design Notes

- All queue storage is allocated at compile time via the `ADD_QUEUE` macro.
- Enqueue/dequeue operations signal associated events, enabling event-driven I/O without polling.

---

### 4.5 Precision Timer

**Files:** [sys/tmr.h](../sys/tmr.h), [sys/tmr.c](../sys/tmr.c)

#### 4.5.1 Responsibilities

- Provides a mechanism for FSM delays with approximately millisecond (1024 ticks/sec) precision by default using a 32 bit hardware counter
- Selectable precision determined by the RTC divider
- Signal associated event when tick count reaches 0

#### 4.5.2 Data Structures

| Structure | Description |
|-----------|-------------|
| `timer_t`   | RAM based collection of data describing the timer's state. This includes the original duration and (via the module tick clock) the ticks remaining |
| `timerDescr_t` | Flash based description of the timer including its name, a pointer to the `timer_t` state in RAM, and a pointer to the associated Event |

#### 4.5.3 Key Interfaces

| Function / Macro | Description |
|-----------------|-------------|
| `ADD_TMR(name, [handler])` | Declare and statically allocate a timer; auto-creates its event with an optional dispatch-context handler |
| `ADD_TMR_ISR(name, handler)` | Declare a timer whose expiry callback runs in interrupt context (least jitter) |
| `bool tmrSet(name, ticks)` | Arm the timer to trigger its associated event after the given ticks |
| `uint32_t tmrGet(name)` | Get the ticks remaining |
| `bool tmrCancel(name)` | Cancel a running timer and disarm its event |
| `evntState_t tmrWait(name, resumeState)` | Suspend the calling FSM until the timer expires, resuming at `resumeState` |

#### 4.5.4 Design Notes

- Cascades the RTC timer using the AVR events system with a 16 bit TCB timer to create a 32 bit counter
- Uses the rtc and tcb drivers to access the hardware functionality
- By default the RTC timer uses an internal clock source providing 1024 ticks/second
- Defines in avrOSConfig.h provide options for different clock sources and different frequencies
- When the user creates a timer an associated avrOS event is created as well
- Uses avrOS events to restore the FSM to the Ready queue when the timer expires
- Provides event or interrupt call back handlers to note the event and possibly reschedule the FSM
- The call back function can be in either the interrupt context for less jitter. Or, It can be in the event call back context which is in the system/FSM context therefore not requiring thread safety with the rest of the FSMs and events
- The callback context is chosen by the associated event: `ADD_TMR` registers a dispatch-context handler (via `ADD_EVENT`), while `ADD_TMR_ISR` registers an interrupt-context handler (via `ADD_EVENT_ISR`, see §4.3)
- Maintains a list of the current active timers
- The next timer to expire is used to calculate the two 16 bit compare registers (TCB and RTC) to generate an interrupt that will trigger that timer's event
- The interrupt will be two stages. Only one comparator interrupt is enabled at a time. First the TCB compare interrupt is enabled and triggered, then the RTC compare interrupt is enabled and triggered
- When that timer expires any remaining timer counters will be deducted the appropriate amount and the next timer to expire will be determined and the compare registers will be set
- If a timer is added while existing timers are counting down, the ticks until the next timer expiration will be calculated and stored in the deduct value for the timer. Otherwise, the deduct value is the number of ticks from the previous interrupt
- If a timer is added while existing timers are counting down and the new timer becomes the next timer to expire, the TCB and RTC compare registers and the deduct value for all of the timers are recalculated

---

### 4.6 File I/O Abstraction (fio)

**Files:** [sys/fio.h](../sys/fio.h)

#### 4.6.1 Responsibilities

- Bridge AVR-libc `FILE` streams to avrOS queues.
- Allow standard C I/O functions (`fprintf`, `fgetc`, etc.) to work with UART and other buffered peripherals.
- Provide blocking wait helpers that use the event system.

#### 4.6.2 Data Structures

| Structure | Description |
|-----------|-------------|
| `fioBuffers_t` | Holds pointers to input and output `queue_t` instances for a `FILE` stream. |

#### 4.6.3 Key Interfaces

| Macro / Function | Description |
|-----------------|-------------|
| `fioGetInputQueue(file)` | Return the input queue for a stream. |
| `fioGetOutputQueue(file)` | Return the output queue for a stream. |
| `fioSetInputQueue(file, que)` | Assign an input queue to a stream. |
| `fioSetOutputQueue(file, que)` | Assign an output queue to a stream. |
| `fioWaitInput(file)` | Event-wait until input queue is not empty. |
| `fioWaitOutput(file)` | Event-wait until output queue is empty. |
| `fioBusyWaitInput(file)` | Polling wait until input queue is not empty. |
| `fioBusyWaitOutput(file)` | Polling wait until output queue is empty. |

---

## 5. Services

### 5.1 Command Line Interface (cli)

**Files:** [srv/cli.c](../srv/cli.c), [srv/cli.h](../srv/cli.h)

#### 5.1.1 Responsibilities

- Provide an interactive, line-oriented command interpreter over a UART stream.
- Support command registration, argument parsing, command history (single level), subcommands, and repeatable commands.
- Allow any module to add commands to the CLI table at compile time.

#### 5.1.2 Data Structures

| Structure | Description |
|-----------|-------------|
| `cliInstance_t` | Runtime instance of a CLI: name, I/O FILE pointers, UART reference, parser state. |
| `cliCommand_t` | Compile-time command descriptor: command string, handler function pointer, repeatable flag, root command pointer. |
| `cliState_t` | Parser state: input buffer, previous command buffer, argument vector, current command pointer. |

#### 5.1.3 Registration Macros

| Macro | Description |
|-------|-------------|
| `ADD_CLI(name, file)` | Create a CLI instance backed by a FILE stream; registers the CLI FSM. |
| `ADD_COMMAND(str, fn, ...)` | Add a top-level command with handler `fn`. |
| `ADD_SUBCOMMAND(str, fn, root, ...)` | Add a subcommand under a `root` command. |

#### 5.1.4 Key Interfaces

| Function | Description |
|----------|-------------|
| `int cliCallFunction(char *cmdLine)` | Parse and execute a command line string programmatically. |

#### 5.1.5 Design Notes

- The CLI runs as an `FSM_SRV`-priority state machine.
- ANSI/VT100 escape sequences are supported for cursor movement and terminal formatting.
- All commands are placed in the `CLI_CMDS` linker section; no runtime registration is required.

---

### 5.2 Logging Service (log)

**Files:** [srv/log.c](../srv/log.c), [srv/log.h](../srv/log.h)

#### 5.2.1 Responsibilities

- Provide compile-time-configurable log message output at four severity levels.
- Route log output to a designated `FILE` stream (typically a UART).
- Optionally include tick count, FSM name, state name, source file, and line number in messages.

#### 5.2.2 Severity Levels

| Level | Macro | Description |
|-------|-------|-------------|
| 4 | `INFO(fmt, ...)` | Informational messages |
| 3 | `WARN(fmt, ...)` | Warning conditions |
| 2 | `ERROR(fmt, ...)` | Non-fatal errors |
| 1 | `CRITICAL(fmt, ...)` | Fatal errors; halts the system |

#### 5.2.3 Configuration

| Macro | Description |
|-------|-------------|
| `LOG_LEVEL` | Compile-time threshold; messages below this level compile to nothing. |
| `LOG_FORMAT` | Selects message format: 1=label only, 2=tick+label, 3=tick+FSM+state, 4=tick+file+line. |

#### 5.2.4 Registration

| Macro | Description |
|-------|-------------|
| `ADD_LOG(name, file)` | Create a log instance; registers an initializer that connects the stream. |

#### 5.2.5 Key Interfaces

| Function | Description |
|----------|-------------|
| `void logRam()` | Print RAM usage summary to the log stream. |
| `void logRom()` | Print ROM usage summary to the log stream. |
| `void logNewLine()` | Emit a newline/carriage-return to the log stream. |

---

### 5.3 PCM Audio Service (pcm)

**Files:** [srv/pcm.c](../srv/pcm.c)

#### 5.3.1 Responsibilities

- Stream PCM audio samples to the DAC driver.
- Manage sample buffering and playback timing synchronized to the system tick.

#### 5.3.2 Design Notes

- This module works in conjunction with the DAC driver ([Section 6.4](#64-dac-driver-dac)).
- Audio data can be converted from WAV or sound files using the `wav2c` and `snd2c` utilities in [util/](../util/).

---

### 5.4 Unit Test Service (uts)

**Files:** [srv/uts.c](../srv/uts.c), [srv/uts.h](../srv/uts.h)

#### 5.4.1 Responsibilities

- Provide a lightweight, self-contained unit-test runner for on-target (or
  simulator) verification that does **not** depend on the FSM scheduler.
- Collect developer-registered test functions from a flash-resident table and
  run them in sequence directly from the `main()` of a unit test project.
- Report each test's name, pass/fail result (color-coded), and a short
  description of the returned `osStatus_t` code over the same UART used by the
  CLI in the example application.
- Summarize the run as a single "Test Group" pass/fail line, publish the group
  result to a well-known global variable, and then halt in an infinite loop so
  the result can be observed by a human, a debugger, or an automated harness.

Because the runner takes over the main thread and never returns, a firmware
image runs *either* an application (FSM main loop) *or* the unit-test service —
not both at once. The two are separated by **project**, not by a compile-time
switch: unit tests live in their own application directory alongside the example
application (see [Configuration](#544-configuration)). An application project
such as `app/avrOS_example` never calls `utsRun()`; a unit test project such as
`app/avrOS_test` calls `utsRun()` in place of the `fsmDispatch()` loop.

#### 5.4.2 Data Structures

The service source defines the descriptor that `ADD_TEST` places in the
`TEST_TABLE` linker section. Conceptually the set of descriptors forms a global
array of `test_t` that the runner walks.

| Structure / Type | Description |
|------------------|-------------|
| `utsTest_t` | Test function pointer type: `typedef osStatus_t (*utsTest_t)(void);`. A test returns `OS_OK` on success or a negative `osStatus_t` on failure. |
| `test_t` | Flash-resident descriptor for one unit test. Contains the test's string `name` and a `func` pointer of type `utsTest_t`. Placed in `TEST_TABLE`. |

```c
typedef osStatus_t (*utsTest_t)(void);   // a unit test: no args, osStatus_t result

typedef struct TEST_TYPE
{
    const char  *name;   // human-readable test name shown in the report
    utsTest_t    func;   // the test function to invoke
} test_t;
```

Runtime state owned by the service:

| Variable | Description |
|----------|-------------|
| `osStatus_t utsResults[UTS_MAX_TESTS]` | Return code recorded for each test, in table order. |
| `volatile int8_t utsGroupResult` | Group result, **initialized to `0`**. Set to `1` if every test returned `OS_OK`, or `-1` if any test failed. Exported so a debugger/harness can read the outcome at the halt loop. |

#### 5.4.3 Registration Macro

`ADD_TEST` follows the same pattern as the other `ADD_*` macros (see
[Section 3.5](#35-linker-sections-and-descriptor-tables)): it emits a `const`
`test_t` descriptor into the `TEST_TABLE` section, marked with
`SECTION(TEST_TABLE)` so `--gc-sections` does not discard it. The tests
themselves are ordinary functions written in the unit test project's `main`
source file.

| Macro | Description |
|-------|-------------|
| `ADD_TEST(name, func)` | Register a unit test. `name` is a string literal shown in the report; `func` is the `utsTest_t` test function. |

```c
// In the unit test project's main source file (same file as main()):
ADD_TEST("queue put/get", testQueuePutGet);
ADD_TEST("timer expiry",  testTimerExpiry);

osStatus_t testQueuePutGet(void)
{
    // ... exercise the code under test ...
    return (ok ? OS_OK : OS_ERROR);
}
```

#### 5.4.4 Key Interfaces

| Function / Variable | Description |
|---------------------|-------------|
| `void utsRun(void)` | Entry point, called directly from a unit test project's `main()` in place of the `fsmDispatch()` loop. Walks `TEST_TABLE`, runs and reports every test, prints the group summary, sets `utsGroupResult`, then enters an infinite `while(1)` loop. Marked `__attribute__((noreturn))`; it never returns. |
| `const char* utsStatusDescription(osStatus_t status)` | Map an `osStatus_t` value to the short human-readable description taken from the `osStatus_t` typedef comments in [avrOS.h](../avrOS.h) (e.g. `OS_INVALID` → "Invalid argument: NULL pointer, out-of-range value, bad enum"). Used when printing each test result. |
| `volatile int8_t utsGroupResult` | Global group result (see [Data Structures](#542-data-structures)). |

Runner algorithm:

1. Initialize the shared UART (the CLI UART) for blocking, polled output.
2. Walk `TEST_TABLE` from `__start_TEST_TABLE` to `__stop_TEST_TABLE`. For each
   descriptor:
   1. Call `descr->func()` and store the returned `osStatus_t` in
      `utsResults[i]`.
   2. Print a line with the test `name` and the result — the word `PASS`
      in green (ANSI) when the code is `OS_OK`, or `FAIL` in red otherwise —
      followed by the short description from `utsStatusDescription()`.
3. After the last test, walk `utsResults[]`. If every entry is `OS_OK`, print
   `Test Group [PASSED]` with `PASSED` in green and set `utsGroupResult = 1`.
   Otherwise print `Test Group [FAIL]` with `FAIL` in red and set
   `utsGroupResult = -1`.
4. Enter `while(1);` and never return.

#### 5.4.5 Configuration

There is no compile-time enable for the service. A project runs unit tests by
calling `utsRun()` from its `main()`; a project that does not call it links no
part of the runner, because `--gc-sections` discards the unreferenced code and
its `utsResults[]` array.

Unit tests are therefore built as their own project directory under `app/`,
parallel to the example application, created with the makefile's `project`
target. The reference unit test project is `app/avrOS_test`:

| File | Content |
|------|---------|
| `main.c` | The `ADD_TEST` registrations, the test functions, and a `main()` that calls `sysInit()` then `utsRun()` — no `fsmDispatch()` loop. |
| `avrOSConfig.h` | As the example application, except the CLI is left undefined and `LOG_LEVEL` is `0` — both need the dispatch loop the runner never enters, and the CLI shares the report USART. |
| `avrOS.x` | Unchanged from the example; it already reserves `TEST_TABLE`. |
| `makefile` | Unchanged from the example. |

Report UART settings, selected in the project's `avrOSConfig.h` and mirroring
the CLI/logger configuration style:

| Macro | Default | Effect |
|-------|---------|--------|
| `UTS_USART` | `CLI_USART` | USART peripheral used for report output — the same UART the example application gives the CLI. |
| `UTS_BAUDRATE` / `UTS_PARITY` / `UTS_DATA_BITS` / `UTS_STOP_BITS` | same as CLI | Serial framing for the report UART. |
| `UTS_MAX_TESTS` | `32` | Size of the `utsResults[]` array; must be ≥ the number of registered tests. |

#### 5.4.6 Report Format

```
queue put/get .......... [PASS] Success
timer expiry ........... [FAIL] Invalid argument: NULL pointer, out-of-range value, bad enum
Test Group [PASSED]
```

`PASS`/`PASSED` render in green and `FAIL` in red on an ANSI terminal, reusing
the color macros already defined for the CLI/logger.

#### 5.4.7 Design Notes

- **No FSM dependency.** `utsRun()` runs on the bare main thread before (in
  place of) the scheduler. It therefore emits output with **blocking, polled**
  UART writes via the [UART driver](#63-uart-driver-uart), bypassing the
  queue/`fio`/FSM-drained path the CLI normally uses — there is no dispatch loop
  running to drain a TX queue.
- **Separated by project, not by `#ifdef`.** The runner is selected by which
  `main()` calls it, so an application project carries no test scaffolding and a
  test project carries no application. Nothing in `main()` is gated on a
  compile-time switch, and application code never calls `utsRun()`.
- **Shared UART, mutually exclusive with the CLI.** The test report and the CLI
  use the same USART, and the runner never yields to the dispatch loop the CLI
  needs. A unit test project therefore leaves the CLI (and the logger)
  unconfigured.
- **Table-driven, order-preserving.** Tests run in the order the linker places
  their descriptors in `TEST_TABLE` (see [Section 3.5](#35-linker-sections-and-descriptor-tables)).
  No test count is hard-coded in the runner; it is derived from the
  `__start_TEST_TABLE` / `__stop_TEST_TABLE` boundary symbols.
- **Machine-readable outcome.** `utsGroupResult` starts at `0` (never ran) and
  becomes `1` (all passed) or `-1` (one or more failed). A simulator or debugger
  can break at the halt loop and read this symbol to score the run without
  parsing UART text.
- **Halt-on-completion.** The terminal `while(1);` keeps the final report and
  `utsGroupResult` stable and prevents the device from running undefined
  application state after the tests complete.

---

## 6. Drivers

### 6.1 CPU Driver (cpu)

**Files:** [drv/cpu.c](../drv/cpu.c), [drv/cpu.h](../drv/cpu.h)

#### 6.1.1 Responsibilities

- Configure the AVR-Dx internal high-frequency oscillator and clock prescaler.
- Optionally route the clock signal to an external pin.
- Provide a software reset function.
- Expose interrupt enable/disable macros.

#### 6.1.2 Key Interfaces

| Function / Macro | Description |
|-----------------|-------------|
| `void cpuSetOSCHF(freq, prescalerEnable, prescaler)` | Configure the internal HF oscillator and prescaler. |
| `uint16_t cpuGetFrequency()` | Calculate and return the current CPU frequency (kHz); returns 0 for external clock. |
| `void cpuClockOut(bool enable)` | Enable or disable clock output to an external pin. |
| `void cpuReset()` | Trigger a software reset. |
| `DISABLE_INTERRUPTS()` | Disable global interrupts (`cli()`). |
| `ENABLE_INTERRUPTS()` | Enable global interrupts (`sei()`). |

---

### 6.2 Memory Driver (mem)

**Files:** [drv/mem.c](../drv/mem.c), [drv/mem.h](../drv/mem.h)

#### 6.2.1 Responsibilities

- Provide compile-time and runtime memory layout information for both RAM and ROM.
- Track maximum stack usage via stack-fill watermarking.
- Generate memory usage reports to a FILE stream.

#### 6.2.2 Key Interfaces

| Function | Description |
|----------|-------------|
| `void memStackFill()` | Fill the free stack area with a known pattern for watermark tracking. |
| `uint16_t memStackSizeMax()` | Return the maximum observed stack depth since `memStackFill()` was called. |
| `void memRomStatus(FILE *file)` | Print ROM region sizes (text, const, rodata, OS tables) to `file`. |
| `void memRamStatus(FILE *file)` | Print RAM region sizes (data, heap, stack, free) to `file`. |

#### 6.2.3 Inline Queries

| Inline Function | Description |
|----------------|-------------|
| `memProgramRomSize()` | Total program flash size. |
| `memConstRomSize()` | Mapped program memory (const ROM) size. |
| `memTextSize()` | Size of the `.text` segment. |
| `memDataSize()` | Size of the initialized data segment. |
| `memHeapSize()` | Current heap allocation. |
| `memStackSize()` | Current stack depth. |
| `memFreeSize()` | Free RAM between heap and stack. |
| `memRamSize()` | Total RAM size. |

---

### 6.3 UART Driver (uart)

**Files:** [drv/uart.c](../drv/uart.c), [drv/uart.h](../drv/uart.h)

#### 6.3.1 Responsibilities

- Provide interrupt-driven, queue-buffered transmit and receive for AVR-Dx USART peripherals.
- Expose AVR-libc `FILE` streams per UART instance for use with standard C I/O functions.
- Collect optional per-instance statistics (byte counts, overflow counts, framing/parity errors).

#### 6.3.2 Data Structures

| Structure | Description |
|-----------|-------------|
| `UART_t` | Compile-time UART descriptor: USART register pointer, baud rate, parity, data bits, stop bits, TX/RX queue pointers, FILE stream pointer, optional stats pointer. |
| `UartStats_t` | Per-instance statistics counters (conditionally compiled with `UART_STATS`). |

#### 6.3.3 Registration Macros

| Macro | Description |
|-------|-------------|
| `ADD_UART_RW(name, reg, baud, parity, data, stop, txSz, rxSz)` | Full-duplex UART with TX and RX queues and a `_FDEV_SETUP_RW` FILE stream. |
| `ADD_UART_WRITE(name, reg, baud, parity, data, stop, txSz)` | TX-only UART. |
| `ADD_UART_READ(name, reg, baud, parity, data, stop, rxSz)` | RX-only UART. |
| `ADD_UART_RAW(name, reg, baud, parity, data, stop)` | Raw UART without queues or FILE stream. |

#### 6.3.4 Key Interfaces

| Function | Description |
|----------|-------------|
| `int uartPutChar(char c, FILE *stream)` | AVR-libc put callback; enqueues a byte for transmission. |
| `int uartGetChar(FILE *stream)` | AVR-libc get callback; dequeues a received byte. |
| `int uartTransmit(uart, buf, n)` | Transmit `n` bytes from `buf`. |
| `int uartTransmitStr(uart, str)` | Transmit a null-terminated string. |
| `int uartReceive(uart, buf, n)` | Receive up to `n` bytes into `buf`. |
| `bool uartRxEmpty(uart)` / `uartTxEmpty(uart)` | Queue empty status queries. |
| `uint8_t uartRxCount(uart)` / `uartTxCount(uart)` | Current queue occupancy. |

#### 6.3.5 Inline USART Register Accessors

The buffered driver above is built on a layer of low-overhead `static inline`
USART register accessors (see §6.6) that operate on a `USART_t *`. New code that
touches the USART registers should use these rather than accessing the registers
directly. They cover baud and frame-format configuration (`usartSetBaud`,
`usartSetFrameFormat`, and the granular `usartSetParity` / `usartSetDataBits` /
`usartSetStopBits` / `usartSetCommMode`), receiver/transmitter and Rx-mode
control, the `usartInt_t` interrupt enables/disables, data transfer
(`usartWriteData` / `usartReadData` / `usartReadRxStatus`), and status helpers
(`usartRxComplete`, `usartDataRegisterEmpty`, `usartTransmitComplete`).

---

### 6.4 DAC Driver (dac)

**Files:** [drv/dac.h](../drv/dac.h)

A header-only inline register driver for the AVR-Dx 10-bit DAC (see §6.6). The
former `dac.c` and its `dacInit()` / `dacOutput()` functions were removed; the
sample-scaling and clamping logic now lives in the PCM service (§5.3), and the
reference selection delegates to the VREF driver (§6.17).

#### 6.4.1 Responsibilities

- Configure and control the single `DAC0` peripheral via inline accessors.
- Select the analog output buffer, standby behavior, and enable state.
- Write the (left-justified) 10-bit conversion data register.

#### 6.4.2 Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `DAC_MAX` | `0x03FF` | Maximum DAC output code (full scale) |
| `DAC_MID` | `0x01FF` | Midpoint DAC output code |
| `DAC_MIN` | `0x0000` | Minimum DAC output code (zero) |

#### 6.4.3 Key Interfaces

| Function | Description |
|----------|-------------|
| `void dacSetReference(VREF_REFSEL_t vRef)` | Select the DAC reference (wrapper around `vrefSetReference(VREF_DAC0, …)`). |
| `void dacOutputBufferEnable(bool enable)` | Connect/release the analog output buffer pin. |
| `void dacEnable(void)` / `dacDisable(void)` | Enable or disable the DAC. |
| `bool dacIsEnabled(void)` | Report the enable state. |
| `void dacRunStandby(bool enable)` | Keep the DAC running in standby sleep. |
| `void dacSetData(uint16_t data)` | Write the DATA register (10-bit code left-justified in bits [15:6]). |

---

### 6.5 GPIO Driver (gpio)

**Files:** [drv/gpio.c](../drv/gpio.c), [drv/gpio.h](../drv/gpio.h)

#### 6.5.1 Responsibilities

- Manage AVR-Dx port pins as named GPIO instances.
- Support output set, clear, toggle, and write operations using port bit-mask registers.
- Support input read operations.
- Bind an optional event to a pin, triggered by the port ISR on a configurable edge/level condition.
- Collect optional toggle statistics per instance.

#### 6.5.2 Data Structures

| Structure | Description |
|-----------|-------------|
| `gpio_t` | Compile-time GPIO descriptor: PORT pointer, pin bit mask, direction, optional event pointer, event sub-type, optional stats pointer. |
| `gpioStats_t` | Toggle counter (conditionally compiled with `GPIO_STATS`). |

#### 6.5.3 Pin, Direction, and Event Types

| Type | Values |
|------|--------|
| `gpioPin_t` | `GPIO_PIN_0` … `GPIO_PIN_7` (individual pin bit masks) |
| `gpioDirection_t` | `GPIO_OUTPUT`, `GPIO_INPUT` |
| `gpioEventType_t` | `GPIO_EVENT_NONE = 0`, `GPIO_EVENT_BOTHEDGES = 1`, `GPIO_EVENT_RISING = 2`, `GPIO_EVENT_FALLING = 3`, `GPIO_EVENT_LEVEL_LOW = 4` |

#### 6.5.4 Registration Macro

`ADD_GPIO(...)` is a variadic dispatcher that selects one of three forms based on argument count:

| Form | Use |
|------|-----|
| `ADD_GPIO(name, port, pin, dir)` | Plain GPIO, no event. |
| `ADD_GPIO(name, port, pin, dir, eventType)` | Event-driven GPIO. Creates `name##_event`, binds the default `evntHandler`. ISR triggers `name##_event` with `eventType` as the sub-type. Configures the pin's ISC from `eventType` at init time. |
| `ADD_GPIO(name, port, pin, dir, eventType, handler)` | Same as above but installs `handler` (`int (*)(volatile event_t *)`) on `name##_event`. |

Consumer pattern for an event-driven GPIO:

```c
ADD_GPIO(Button, PORTA, GPIO_PIN_2, GPIO_INPUT, GPIO_EVENT_FALLING);
// in an FSM state (resume in buttonPressed when the event fires):
evntWait(evntGetEvent("Button_event"), GPIO_EVENT_FALLING, buttonPressed);
```

#### 6.5.5 Key Interfaces

| Function | Description |
|----------|-------------|
| `void gpioSetOutput(gpio, value)` | Set pin(s) high using `OUTSET`. |
| `void gpioClearOutput(gpio, value)` | Clear pin(s) low using `OUTCLR`. |
| `void gpioToggleOutput(gpio, value)` | Toggle pin(s) using `OUTTGL`. |
| `void gpioWriteOutput(gpio, value)` | Write masked value to port `OUT` register. |
| `uint8_t gpioReadInput(gpio)` | Read and mask port `IN` register. |
| `uint8_t gpioReadOutput(gpio)` | Read and mask port `OUT` register. |

> The GPIO driver's register accesses are implemented on top of the I/O Port
> driver (§6.24); it remains the higher-level, descriptor-based interface with
> named instances and event callbacks.

---

### 6.6 Inline Register Drivers — Overview

**Files:** [drv/clk.h](../drv/clk.h), [drv/slp.h](../drv/slp.h), [drv/rst.h](../drv/rst.h), [drv/wdt.h](../drv/wdt.h), [drv/nvm.h](../drv/nvm.h), [drv/int.h](../drv/int.h), [drv/tca.h](../drv/tca.h), [drv/tcb.h](../drv/tcb.h), [drv/rtc.h](../drv/rtc.h), [drv/evt.h](../drv/evt.h), [drv/vref.h](../drv/vref.h), [drv/adc.h](../drv/adc.h), [drv/ac.h](../drv/ac.h), [drv/zcd.h](../drv/zcd.h), [drv/spi.h](../drv/spi.h), [drv/twi.h](../drv/twi.h), [drv/pmux.h](../drv/pmux.h), [drv/pio.h](../drv/pio.h)

Sections 6.7–6.24 document a family of **header-only register drivers**: each
provides `static inline` functions that wrap the registers of one AVR-Dx
peripheral so that application, service, and kernel code never manipulates those
registers directly. This is a project convention — new code must use the matching
`drv/<periph>.h` driver rather than `PERIPH.REG` accesses, and drivers delegate
to each other for shared resources (e.g. ADC/AC/DAC reference selection calls the
VREF driver).

Common design rules across these drivers:

- **Zero overhead.** Functions are `static inline` register pokes; at the
  optimization levels used they compile to the same instructions as direct
  register access.
- **Instance handling.** Multi-instance peripherals (TCA, TCB, AC, ZCD, SPI,
  TWI, PORT) take a caller-supplied pointer (`TCB_t *`, `PORT_t *`, …). Singleton
  peripherals (RTC, EVSYS, CPUINT, VREF, ADC, DAC, CLKCTRL, SLPCTRL, RSTCTRL,
  WDT, NVMCTRL, PORTMUX) operate directly on the global peripheral.
- **Field setters preserve neighbors.** Setters for one field perform a
  read-modify-write that leaves the other fields in the register unchanged;
  dedicated set/clear/toggle/strobe registers are used where the hardware
  provides them (PORT, TCA CTRLE).
- **Interrupt-flag selectors.** Where a peripheral has multiple interrupt
  sources, an enum mask type (`tcbInt_t`, `adcInt_t`, …) drives uniform
  enable/disable/get/clear functions.
- **Configuration Change Protection (CCP).** Drivers for CCP-protected
  registers (CLKCTRL, RSTCTRL, WDT, NVMCTRL, CPUINT vector table) perform the
  protected write internally via `ccp_write_io()` / `ccp_write_spm()`.
- **Clock-domain synchronization.** Drivers whose registers cross a clock domain
  (RTC, WDT) expose busy/sync helpers and block in their setters until a prior
  write has synchronized.
- **Interrupt-safety contract.** Each header documents an "Interrupt safety"
  section: configuration setters are not interrupt-safe (8-bit read-modify-write)
  and should be serialized with `ATOMIC_BLOCK` if shared with an ISR; 16-bit
  count/result registers accessed through a shared `TEMP` register additionally
  provide atomic read variants (e.g. `tcbGetCountAtomic`, `adcGetResultAtomic`).

Per-function documentation lives in the Doxygen comments in each header; the
sections below summarize each driver's scope and principal interfaces.

---

### 6.7 Clock Driver (clk)

**Files:** [drv/clk.h](../drv/clk.h) — singleton (`CLKCTRL`)

#### 6.7.1 Responsibilities

- Select the main clock source and prescaler.
- Configure the internal HF oscillator (frequency, auto-tune, run-standby), the
  internal/external 32.768 kHz oscillators, and the PLL.
- Control the clock-output pin, the configuration lock, and read oscillator
  status. CCP-protected registers are written through the protected sequence.

#### 6.7.2 Key Interfaces

| Group | Functions |
|-------|-----------|
| Main clock | `clkSetSource`, `clkGetSource`, `clkSetPrescaler`, `clkPrescalerEnabled`, `clkGetPrescaler`, `clkClockOut` |
| HF oscillator | `clkSetOscHFFrequency`, `clkGetOscHFFrequency`, `clkOscHFAutotune`, `clkOscHFRunStandby` |
| 32 kHz / PLL | `clkOsc32kRunStandby`, `clkXosc32kEnable`, `clkXosc32kExternalClock`, `clkPllSetMultiplier`, `clkPllRunStandby` |
| Lock / status | `clkLock`, `clkIsLocked`, `clkGetStatus`, `clkStatusReady` (`clkStatus_t`) |

The CPU driver (§6.1) is implemented on top of this driver.

---

### 6.8 Sleep Driver (slp)

**Files:** [drv/slp.h](../drv/slp.h) — singleton (`SLPCTRL`)

#### 6.8.1 Responsibilities

- Select the sleep mode, enable sleep, and execute the `sleep` instruction.
- Configure the voltage-regulator performance and high-temperature low-leakage
  options.

#### 6.8.2 Key Interfaces

| Function | Description |
|----------|-------------|
| `slpSetMode` / `slpGetMode` | Select / read the sleep mode (`SLPCTRL_SMODE_*_gc`). |
| `slpEnable` / `slpEnter` | Arm sleep / execute the `sleep` instruction. |
| `slpSleep(mode)` | Combined enable → sleep → disable cycle (used by `sysSleep()`). |
| `slpSetPerformanceMode` / `slpHighTempLowLeakage` | Voltage-regulator options. |

---

### 6.9 Reset Driver (rst)

**Files:** [drv/rst.h](../drv/rst.h) — singleton (`RSTCTRL`)

#### 6.9.1 Responsibilities

- Read and clear the reset-source flags that record the cause of the last reset.
- Issue a CCP-protected software reset.

#### 6.9.2 Key Interfaces

| Function | Description |
|----------|-------------|
| `rstGetFlags` / `rstClearFlags` | Read / clear reset-source flags (`rstFlag_t`). |
| `rstSoftwareReset` | Trigger a software reset (CCP-protected). Used by `cpuReset()`. |

---

### 6.10 Watchdog Driver (wdt)

**Files:** [drv/wdt.h](../drv/wdt.h) — singleton (`WDT`)

#### 6.10.1 Responsibilities

- Set the time-out period and optional closed window, clear (kick) the watchdog,
  and lock the configuration. CTRLA is CCP-protected and clock-synchronized.

#### 6.10.2 Key Interfaces

| Function | Description |
|----------|-------------|
| `wdtReset` | Clear the watchdog (`wdr` instruction); safe in any context. |
| `wdtSetPeriod` / `wdtSetWindow` / `wdtEnable` / `wdtDisable` | Period / window configuration (blocking on sync, CCP-protected). |
| `wdtSyncBusy` / `wdtWaitSync` | Synchronization helpers. |
| `wdtLock` / `wdtIsLocked` | Lock the configuration until reset. |

---

### 6.11 Non-Volatile Memory Driver (nvm)

**Files:** [drv/nvm.h](../drv/nvm.h) — singleton (`NVMCTRL`)

#### 6.11.1 Responsibilities

- Issue flash/EEPROM commands (CCP SPM-protected), poll busy/ready and error
  status, control the EEPROM-ready interrupt, and configure flash-to-data-space
  mapping. The page-buffer writes themselves are the caller's responsibility.

#### 6.11.2 Key Interfaces

| Function | Description |
|----------|-------------|
| `nvmCommand` / `nvmClearCommand` | Issue / clear an NVM command (`NVMCTRL_CMD_*_gc`, CCP SPM-protected). |
| `nvmWaitReady` / `nvmFlashBusy` / `nvmEepromBusy` / `nvmGetStatus` | Busy/ready status (`nvmBusy_t`). |
| `nvmGetError` / `nvmEepromReady` | Error code and EEPROM-ready flag. |
| `nvmEnableEepromReadyInterrupt` | EEPROM-ready interrupt enable. |
| `nvmSetFlashMap` / `nvmLockFlashMap` | Flash-section data-space mapping. |

---

### 6.12 Interrupt Controller Driver (int)

**Files:** [drv/int.h](../drv/int.h) — singleton (`CPUINT`)

#### 6.12.1 Responsibilities

- Configure interrupt priority scheduling (round-robin, the round-robin/low
  vector, and the single level-1 high-priority vector), the CCP-protected
  vector-table options, and read the execution-status flags. The global interrupt
  enable (SREG I-bit) remains in the CPU driver (§6.1).

#### 6.12.2 Key Interfaces

| Function | Description |
|----------|-------------|
| `intRoundRobinEnable` / `intSetRoundRobinPriority` / `intSetHighPriorityVector` | Priority scheduling. |
| `intVectorsInBoot` / `intCompactVectorTableEnable` | CCP-protected vector-table configuration. |
| `intGetStatus` / `intLevel0Executing` / `intLevel1Executing` / `intNMIExecuting` | Execution status (`intStatus_t`). |

---

### 6.13 Timer/Counter Type A Driver (tca)

**Files:** [drv/tca.h](../drv/tca.h) — multi-instance (`TCA_t *`)

#### 6.13.1 Responsibilities

- Configure and control a TCA in both operating modes: **normal** (single 16-bit
  timer with three compare/PWM channels, buffered period/compare registers,
  direction control, and a command strobe) and **split** (two independent 8-bit
  timers). Normal-mode functions are `tca*`; split-mode functions are `tcaSplit*`.

#### 6.13.2 Data Types

| Type | Description |
|------|-------------|
| `tcaChannel_t` | Compare channel selector (`TCA_CHANNEL0`–`2`). |
| `tcaSplitTimer_t` | Split-mode half selector (`TCA_SPLIT_LOW` / `HIGH`). |
| `tcaInt_t` / `tcaSplitInt_t` | Normal / split interrupt-source masks. |

#### 6.13.3 Key Interfaces

| Group | Functions |
|-------|-----------|
| Mode / run | `tcaSetSplitMode`, `tcaEnable`, `tcaDisable`, `tcaSetClock`, `tcaSetMode`, `tcaRunStandby` |
| Count / compare | `tcaSetCount`/`Get`/`GetAtomic`, `tcaSetPeriod`/`Buffer`, `tcaSetCompare`/`Buffer`/`Get`/`GetAtomic`, `tcaEnableCompare` |
| Control | `tcaSetCountDirection`, `tcaCommand`, `tcaLockUpdate`, `tcaSetAutoLockUpdate` |
| Interrupts / events / debug | `tcaEnableInterrupt`/`Disable`/`GetFlags`/`ClearFlags`, `tcaEnableEventCountA/B`, `tcaSetEventActionA/B`, `tcaDebugRun` |
| Split mode | `tcaSplitEnable`/`Disable`/`SetClock`, `tcaSplitSetCount`/`Period`/`Compare`, `tcaSplitEnableCompare`, `tcaSplitCommand`, `tcaSplit*Interrupt*` |

---

### 6.14 Timer/Counter Type B Driver (tcb)

**Files:** [drv/tcb.h](../drv/tcb.h) — multi-instance (`TCB_t *`)

#### 6.14.1 Responsibilities

- Configure a TCB's counter mode and clock, run control, the 16-bit count and
  compare/capture registers, interrupt and event control, the pin output, and
  debug behavior. The system tick (§4.1) is built on a TCB using this driver.

#### 6.14.2 Key Interfaces

| Group | Functions |
|-------|-----------|
| Mode / run | `tcbSetMode`, `tcbSetClock`, `tcbEnable`, `tcbDisable`, `tcbRunStandby`, `tcbIsRunning` |
| Count / compare | `tcbSetCount`/`Get`/`GetAtomic`, `tcbSetCompare`/`GetCapture`/`GetCaptureAtomic` |
| Interrupts / event | `tcbEnableInterrupt`/`Disable`/`GetFlags`/`ClearFlags` (`tcbInt_t`), `tcbEventInputEnable`, `tcbEventEdge`, `tcbEventFilter` |
| Output / debug | `tcbOutputEnable`, `tcbDebugRun` |

---

### 6.15 Real-Time Counter Driver (rtc)

**Files:** [drv/rtc.h](../drv/rtc.h) — singleton (`RTC`)

#### 6.15.1 Responsibilities

- Drive both functional blocks of the RTC: the **RTC counter** (clock/prescaler,
  run control, count/period/compare, overflow and compare-match interrupts,
  crystal error correction) and the **PIT** periodic interrupt timer. The RTC is
  in a separate clock domain, so count-domain setters block on the relevant busy
  flag.

#### 6.15.2 Key Interfaces

| Group | Functions |
|-------|-----------|
| Sync | `rtcSyncBusy`, `rtcWaitSync` (`rtcSync_t`) |
| Counter | `rtcSetClock`, `rtcSetPrescaler`, `rtcEnable`/`Disable`/`IsEnabled`, `rtcRunStandby`, `rtcSetCount`/`Get`/`GetAtomic`, `rtcSetPeriod`/`GetAtomic`, `rtcSetCompare`/`GetAtomic` |
| Interrupts / correction | `rtcEnableInterrupt`/`Disable`/`GetFlags`/`ClearFlags` (`rtcInt_t`), `rtcEnableCorrection`, `rtcSetCalibration`, `rtcDebugRun` |
| PIT | `rtcPitSetPeriod`, `rtcPitEnable`/`Disable`/`IsEnabled`, `rtcPitEnableInterrupt`, `rtcPitGetInterruptFlag`/`ClearInterruptFlag`, `rtcPitSyncBusy`/`WaitSync`, `rtcPitDebugRun` |

---

### 6.16 Event System Driver (evt)

**Files:** [drv/evt.h](../drv/evt.h) — singleton (`EVSYS`)

#### 6.16.1 Responsibilities

- Route hardware event generators to event users through the eight multiplexer
  channels and strobe software events. This is the *hardware* event router and
  is unrelated to the software event manager (§4.3).

#### 6.16.2 Key Interfaces

| Function | Description |
|----------|-------------|
| `evtSetChannelGenerator` / `evtGetChannelGenerator` / `evtDisableChannel` | Route a generator onto a channel (`evtChannel_t`). |
| `evtSetUser` / `evtClearUser` / `evtGetUser` | Connect an event user register to a channel. |
| `evtSoftwareEvent` | Strobe a software event onto a channel. |

---

### 6.17 Voltage Reference Driver (vref)

**Files:** [drv/vref.h](../drv/vref.h) — singleton (`VREF`)

#### 6.17.1 Responsibilities

- Select the reference source/voltage and the always-on power option
  independently for the ADC, DAC, and analog comparator. The DAC, ADC, and AC
  drivers delegate their reference selection here.

#### 6.17.2 Key Interfaces

| Function | Description |
|----------|-------------|
| `vrefSetReference` / `vrefGetReference` | Select / read a peripheral's reference (`vrefPeripheral_t`, `VREF_REFSEL_*_gc`). |
| `vrefAlwaysOn` | Force a reference always on (trade start-up latency vs. power). |

---

### 6.18 ADC Driver (adc)

**Files:** [drv/adc.h](../drv/adc.h) — singleton (`ADC0`)

#### 6.18.1 Responsibilities

- Configure and run the ADC: resolution/justification/mode, prescaler and
  sampling timing, sample accumulation, input multiplexers, conversion
  start/stop and start-event, the window comparator, interrupts, and the result
  register. The reference is delegated to the VREF driver (§6.17).

#### 6.18.2 Key Interfaces

| Group | Functions |
|-------|-----------|
| Run / format | `adcEnable`/`Disable`, `adcFreeRun`, `adcSetResolution`, `adcLeftAdjust`, `adcSetConversionMode`, `adcRunStandby`, `adcSetReference` |
| Timing | `adcSetAccumulation`, `adcSetPrescaler`, `adcSetInitDelay`, `adcSetSampleDelay`, `adcSetSampleLength` |
| Inputs / conversion | `adcSetPositiveInput`, `adcSetNegativeInput`, `adcStartConversion`, `adcStopConversion`, `adcEnableStartEvent` |
| Window / interrupts / result | `adcSetWindowMode`, `adcSetWindowLow`/`High`, `adcEnableInterrupt`/`Disable`/`GetFlags`/`ClearFlags` (`adcInt_t`), `adcResultReady`, `adcGetResult`/`GetResultAtomic`, `adcDebugRun` |

---

### 6.19 Analog Comparator Driver (ac)

**Files:** [drv/ac.h](../drv/ac.h) — multi-instance (`AC_t *`)

#### 6.19.1 Responsibilities

- Configure and control an analog comparator: enable, hysteresis, power profile,
  output/standby, window mode, input multiplexers, DAC reference level, output
  invert, interrupts, and comparator/window status. The shared comparator
  reference is delegated to the VREF driver (§6.17) via `acSetReference()`.

#### 6.19.2 Key Interfaces

| Group | Functions |
|-------|-----------|
| Run / config | `acEnable`/`Disable`, `acSetHysteresis`, `acSetPowerProfile`, `acOutputEnable`, `acRunStandby`, `acSetWindowMode` |
| Inputs / reference | `acSetPositiveInput`, `acSetNegativeInput`, `acInvertOutput`, `acSetReference`, `acSetDacRef` |
| Interrupts / status | `acEnableInterrupt`, `acSetInterruptMode`, `acGetInterruptFlag`/`ClearInterruptFlag`, `acGetState`, `acGetWindowState` |

---

### 6.20 Zero-Cross Detector Driver (zcd)

**Files:** [drv/zcd.h](../drv/zcd.h) — multi-instance (`ZCD_t *`)

#### 6.20.1 Responsibilities

- Enable a zero-cross detector and configure inversion, the output pad, standby
  behavior, the interrupt edge, and read the crossing flag and output state.

#### 6.20.2 Key Interfaces

| Function | Description |
|----------|-------------|
| `zcdEnable` / `zcdDisable` | Enable / disable the detector. |
| `zcdInvert` / `zcdOutputEnable` / `zcdRunStandby` | Polarity, output pad, standby. |
| `zcdSetInterruptMode` | Select the crossing edge (`ZCD_INTMODE_*_gc`). |
| `zcdGetInterruptFlag` / `zcdClearInterruptFlag` / `zcdGetState` | Flag and state. |

---

### 6.21 SPI Driver (spi)

**Files:** [drv/spi.h](../drv/spi.h) — multi-instance (`SPI_t *`)

#### 6.21.1 Responsibilities

- Configure host/client role, clock prescaler and double-speed, data order and
  transfer mode, slave-select and buffer modes, interrupts, and the data
  register; plus a blocking full-duplex byte transfer for normal host mode.

#### 6.21.2 Key Interfaces

| Group | Functions |
|-------|-----------|
| Config | `spiEnable`/`Disable`, `spiHostMode`, `spiSetPrescaler`, `spiDoubleSpeed`, `spiDataOrder`, `spiSetMode`, `spiSlaveSelectDisable`, `spiBufferEnable` |
| Interrupts | `spiEnableInterrupt`/`Disable` (`spiInt_t`), `spiGetInterruptFlags`/`ClearInterruptFlags` |
| Data | `spiWriteData`, `spiReadData`, `spiTransferByte` |

---

### 6.22 TWI Driver (twi)

**Files:** [drv/twi.h](../drv/twi.h) — multi-instance (`TWI_t *`)

#### 6.22.1 Responsibilities

- Configure the shared pin/timing settings and drive the independent host
  (master) and client (slave) controllers: enable, baud/address, command
  strobes, data registers, interrupts, and status. Host functions are
  `twiHost*`; client functions are `twiClient*`.

#### 6.22.2 Key Interfaces

| Group | Functions |
|-------|-----------|
| General | `twiSetSdaHold`, `twiSetSdaSetup`, `twiFastModePlus`, `twiSetInputLevel`, `twiDualModeEnable`, `twiDebugRun` |
| Host | `twiHostEnable`, `twiHostSmartMode`, `twiHostSetTimeout`, `twiHostQuickCommand`, `twiHostEnableRead/WriteInterrupt`, `twiHostCommand`, `twiHostAckAction`, `twiHostFlush`, `twiHostSetBaud`, `twiHostSetAddress`, `twiHostWrite/ReadData`, `twiHostGetStatus`, `twiHostBusState`, `twiHostSetBusState`, `twiHostGotAck`, `twiHostClearFlags` |
| Client | `twiClientEnable`, `twiClientSmartMode`, `twiClientEnableData/Address/StopInterrupt`, `twiClientCommand`, `twiClientAckAction`, `twiClientSetAddress`, `twiClientSetAddressMask`, `twiClientWrite/ReadData`, `twiClientGetStatus`, `twiClientIsRead`, `twiClientGotAck`, `twiClientClearFlags` |

---

### 6.23 Port Multiplexer Driver (pmux)

**Files:** [drv/pmux.h](../drv/pmux.h) — singleton (`PORTMUX`)

#### 6.23.1 Responsibilities

- Select which physical pins each peripheral signal is routed to. One setter per
  routable signal performs a read-modify-write of its field in the route
  register.

#### 6.23.2 Key Interfaces

| Group | Functions |
|-------|-----------|
| Serial | `pmuxUsart0`/`1`/`2`, `pmuxSpi0`/`1`, `pmuxTwi0` |
| Timers | `pmuxTca0`, `pmuxTcb0`/`1`/`2`, `pmuxTcd0` |
| Logic / events / analog | `pmuxCclLut0`–`3`, `pmuxEvOutA`/`C`/`D`, `pmuxAc0`–`2`, `pmuxZcd0` |

---

### 6.24 I/O Port Driver (pio)

**Files:** [drv/pio.h](../drv/pio.h) — multi-instance (`PORT_t *`)

#### 6.24.1 Responsibilities

- Provide low-level PORT register access: pin direction, output value, and input
  reading via the atomic set/clear/toggle registers; pin-change interrupt flags;
  the slew-rate option; and per-pin configuration (sense mode, pull-up, invert).
  The GPIO driver (§6.5) is the higher-level descriptor-based interface built on
  this driver.

#### 6.24.2 Data Types

| Type | Description |
|------|-------------|
| `pioPin_t` | Pin bit-mask selector (`PIO_PIN0`–`7`, `PIO_PIN_ALL`) for bulk operations. Per-pin configuration takes a 0–7 index. |

#### 6.24.3 Key Interfaces

| Group | Functions |
|-------|-----------|
| Direction | `pioSetOutput`, `pioSetInput`, `pioToggleDirection`, `pioWriteDirection`, `pioGetDirection` |
| Output / input | `pioSet`, `pioClear`, `pioToggle`, `pioWrite`, `pioReadOutput`, `pioRead` |
| Interrupt flags / control | `pioGetInterruptFlags`, `pioClearInterruptFlags`, `pioSlewRateLimit` |
| Per-pin config | `pioSetPinConfig`/`GetPinConfig`, `pioSetInputSense`, `pioPullup`, `pioInvert`, `pioConfigPins` |

---

## 7. Configuration

Application-specific configuration is provided in [app/avrOS_example/avrOSConfig.h](../app/avrOS_example/avrOSConfig.h). This file is included by [avrOS.h](../avrOS.h) before all other OS headers.

| Configuration Area | Relevant Macros |
|--------------------|----------------|
| CLI buffer sizes | `MAX_CMD_LINE`, `MAX_ARGS` |
| Logging | `LOG_LEVEL`, `LOG_FORMAT` |
| Diagnostics | `UART_STATS`, `GPIO_STATS`, `FSM_STATS` |
| System tick | `SYS_TICK_TIMER` |

---

## 8. Application Interface

Applications interact with avrOS through [avrOS.h](../avrOS.h), which provides:

- OS-wide constants (`OK`, `ERR`, `ENABLE`, `DISABLE`).
- General utility macros (`UNUSED`, `CONCAT`, `SECTION`, `ROM_STR`, etc.).
- Inline math helpers (`percentWhole()`, `percentPlaces()`).
- Includes of all system, service, and driver headers.

A typical application file structure:

```c
#include "avrOS.h"

// Register peripherals
ADD_UART_RW(console, USART0, 115200, USART_PMODE_DISABLED_gc,
            USART_CHSIZE_8BIT_gc, USART_SBMODE_1BIT_gc, 64, 64);
ADD_CLI(myCli, console_file);
ADD_COMMAND("help", helpCmd);

// Register application FSM
ADD_STATE_MACHINE(myApp, myAppInit, FSM_APP);

int myAppInit(volatile fsmStateMachine_t *sm) { ... }

int main(void)
{
    sysInit();
    while (1)
    {
        fsmDispatch();
        sysSleep();
    }
}
```

---

---

## 9. Verification and Testing

avrOS currently has no automated test suite. Validation is
hardware-in-the-loop. This section captures the practices in use today
and the path forward.

### 9.1 Hardware-in-the-loop bring-up

Standard validation workflow for a code change:

1. `make all` — must build clean, no new cppcheck findings.
2. `make flash` to the reference target (AVR128DA28 on the Pi 4 Dev
   Station; see [PI4_Dev_Station.md](PI4_Dev_Station.md)).
3. Connect to the CLI USART (default 115200 8N1).
4. Confirm the CLI banner appears.
5. `ram` / `rom` — confirm the change did not blow the budget.
6. `fsm` — confirm all expected state machines reach Ready.
7. Run the change-specific CLI command(s) (`gpio`, `que`, `evnt`, …).
8. Soak for several minutes with the `r` repeat modifier on the
   relevant inspection command (e.g. `quer`, `evntr`).

### 9.2 Test hardware

| Item | Notes |
|------|-------|
| AVR128DA28 reference board | The example app is wired for this part. |
| Raspberry Pi 4 Dev Station | Hosts the toolchain and serial UPDI programmer. |
| Two USB-serial adapters | One for logger, one for CLI (when both are enabled). |
| Microchip Atmel-ICE (optional) | Alternative programmer / debugger. |

### 9.3 Manual regression checklist

Before merging a change to `sys/` or `drv/`:

- [ ] `ram` reports stack-max stable across a soak run.
- [ ] `rom` text size has not jumped unexpectedly.
- [ ] `evnt` shows non-zero `Triggered` for any event the change touches.
- [ ] `que` shows zero `Overflow` for any queue the change touches
      under worst-case load.
- [ ] `fsm` shows no state machine permanently stuck in Wait that
      shouldn't be.
- [ ] CLI is responsive after 10 minutes of idle.
- [ ] No new cppcheck findings (`make analyze`).
- [ ] No complexity score newly above 20 (`make complexity`).

### 9.4 Future work — host-side unit tests

Several modules are pure C and would compile against the host
toolchain with thin shims:

1. Create `test/host/` with a `Makefile` using the system `gcc`.
2. Provide stubs for `<avr/io.h>`, `<avr/interrupt.h>`,
   `<util/atomic.h>`, the AVR register layouts, and the linker
   `__start_*` / `__stop_*` symbols (back them with a static array).
3. Adopt **Unity** as the test framework.
4. Cover `sys/queue.c` (put/get/empty/full/wrap/overflow), `sys/event.c`
   (arm → trigger → dispatch → re-arm cycle, list invariants), and
   `sys/fsm.c` (ready-list priority ordering, `fsmReady` /
   `fsmStop` / `fsmReset`).

These modules have no real hardware dependency once `ATOMIC_BLOCK` is
stubbed to a no-op.

### 9.5 Future work — board-in-the-loop CI

Reachable with a Raspberry Pi as build host and programmer:

1. Wire the Pi to the AVR via serial UPDI
   (`PRG = serialupdi -P /dev/ttyAMA2`).
2. `make all flash` from CI on every push to `develop`.
3. A smoke-test Python script issues a fixed CLI command sequence over
   the CLI USART and asserts on the output.

### 9.6 On-target unit tests (uts)

The [Unit Test Service (uts)](#54-unit-test-service-uts) provides an on-target
(or simulator) unit-test runner that complements the host-side approach in §9.4.
Tests live in their own project under `app/`, parallel to the example
application (`app/avrOS_test`). They are ordinary `osStatus_t` functions in that
project's `main` source file, registered with `ADD_TEST`, and executed by
`utsRun()` directly from its `main()` — in place of the FSM dispatch loop, and
without the FSM scheduler. The runner prints a color-coded pass/fail
line and description per test, a `Test Group [PASSED]`/`[FAIL]` summary, and
publishes the outcome to `utsGroupResult` (`1` = all passed, `-1` = failure)
before halting — so a debugger or board-in-the-loop harness (§9.5) can score the
run by reading a single symbol rather than parsing UART text.

---

## 10. Revision History

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0 | 2026-02-24 | John Anderson | Initial outline |
| 1.1 | 2026-05-17 | (consolidation) | Add §3.4 Memory Layout, §3.5 Linker Sections, expanded §4.2 priority detail, §4.3.4 event sub-type contract, §9 Verification & Testing. |
| 1.2 | 2026-06-27 | John Anderson | Add §6.6 inline register driver overview and §6.7–6.24 sections for the new peripheral drivers (clk, slp, rst, wdt, nvm, int, tca, tcb, rtc, evt, vref, adc, ac, zcd, spi, twi, pmux, pio); update §6.3 (UART inline accessors) and §6.4 (DAC converted to header-only inline). |
