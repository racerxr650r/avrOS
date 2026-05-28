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
   - 4.5 [File I/O Abstraction (fio)](#45-file-io-abstraction-fio)
5. [Services](#5-services)
   - 5.1 [Command Line Interface (cli)](#51-command-line-interface-cli)
   - 5.2 [Logging Service (log)](#52-logging-service-log)
   - 5.3 [PCM Audio Service (pcm)](#53-pcm-audio-service-pcm)
6. [Drivers](#6-drivers)
   - 6.1 [CPU Driver (cpu)](#61-cpu-driver-cpu)
   - 6.2 [Memory Driver (mem)](#62-memory-driver-mem)
   - 6.3 [UART Driver (uart)](#63-uart-driver-uart)
   - 6.4 [DAC Driver (dac)](#64-dac-driver-dac)
   - 6.5 [GPIO Driver (gpio)](#65-gpio-driver-gpio)
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
linker script at [app/avrOS_example/avrOS.x](../app/avrOS_example/avrOS.x)
splits flash into an unmapped region (code) and a 32 KiB *flash window*
that the AVR-Dx maps into the low data-address space. Read-only data
and OS descriptor tables live in the window so they can be iterated
through ordinary C pointers without `pgm_read_*()` calls.

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
section in the flash window. The linker script publishes
`__start_<NAME>` / `__stop_<NAME>` symbols around each section so the
OS can iterate them at runtime:

| Section | Producer macro | Iterated by |
|---------|----------------|-------------|
| `CLI_CMDS`    | `ADD_COMMAND`        | `srv/cli.c` |
| `FSM_TABLE`   | `ADD_STATE_MACHINE`, `ADD_INITIALIZER` | `sys/fsm.c`, `sys/sys.c` |
| `QUE_TABLE`   | `ADD_QUEUE`          | `sys/queue.c` |
| `TMR_TABLE`   | (reserved for future timers) | — |
| `EVNT_TABLE`  | `ADD_EVENT`          | `sys/event.c` |
| `GPIO_TABLE`  | `ADD_GPIO`           | `drv/gpio.c` |
| `UART_TABLE`  | `ADD_UART_RW`, `ADD_UART_WO`, `ADD_UART_RO` | `drv/uart.c` |

`SECTION(x)` is `__attribute__((__used__, __section__(#x)))`. The
`__used__` is required — otherwise the linker may discard a descriptor
that has no direct C reference.

To **add a new descriptor table**:

1. Pick a name `<MOD>_TABLE` (uppercase, ends `_TABLE`).
2. Add a block to `avrOS.x` inside the `text_window` group,
   immediately after an existing table. Update the next block's
   `ADDR(...)` to refer to the new table. If your table is the new
   last one, move `__stop_text_window = . ;` into your block.
3. In C, mark the descriptor with `SECTION(<MOD>_TABLE)` and declare
   `extern void *__start_<MOD>_TABLE, *__stop_<MOD>_TABLE;` in the
   module that iterates it.

Rules:

- Runtime mutable data is **never** placed in a `<MOD>_TABLE`. The
  flash window is read-only.
- Section names match exactly across the C source and the linker
  script — every module's CLI / init code depends on the precise
  `__start_*` / `__stop_*` symbol name.

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

- The tick ISR calls `evntTrigger(&tick, EVENT_TYPE_TICK)` to mark the `tick` event triggered.
- The `tick` event is registered with `ADD_EVENT(tick, sysUpdateWaitTicks)`, binding a custom handler. `evntDispatch()` invokes `sysUpdateWaitTicks()` in main-loop context, which decrements wait-tick counters across all FSMs (via `fsmUpdateWaitTicks()`) and re-arms the event via `evntArmSystem()`. This is the canonical example of the self-arming pattern (see §4.3.3).
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
| `ADD_EVENT(name [, handler])` | Define an event object + flash descriptor; binds an optional custom handler (defaults to `evntHandler`). |
| `evntArm(sm, event)` | Move an event from the disarmed list to the armed list and place `sm` in the FSM wait queue. |
| `evntArmSystem(event)` | Arm a system-wide event with no associated state machine. Used by self-arming events (e.g. system tick) whose handler runs directly from `evntDispatch`. |
| `evntDisarm(event)` | Move an event from the armed list back to the disarmed list. |
| `evntWait(sm, event, eventType)` | Set `event->evntType = eventType`, call `evntArm(sm, event)`. The default handler will wake `sm` only when a producer triggers a matching sub-type. |
| `evntTrigger(event, subType)` | ISR-safe. Move the event from armed to triggered, record `subType` in `event->trigger`. |
| `evntDispatch()` | Called by `fsmDispatch()`. Pops each triggered event, calls its handler, then (for FSM-bound events) moves it and any sister events sharing the same `stateMachine` to the disarmed list. |
| `evntGetStatus / evntGetType / evntGetTrigger / evntGetStateMachine` | Inline accessors for handler use. |
| `evntInit()` | Walks the `EVNT_TABLE` and populates the disarmed list. Called by `sysInit()`. |

#### 4.3.3 Design Notes

- **Three global lists.** The event manager owns three intrusive linked lists threaded through `event_t.next`: `evntListDisarmed`, `evntListArmed`, `evntListTriggered`. Every event is on exactly one list at any moment. Lifecycle: `disarmed → armed → triggered → disarmed`.
- **Descriptor / status split.** `evntDescriptor_t` lives in flash (`EVNT_TABLE`) and carries the event name and handler pointer. `event_t` is the RAM status object that carries list linkage, current state, and the `evntType`/`trigger` sub-type pair. `ADD_EVENT(name)` emits both and links them.
- **Handler binding.** The handler pointer is in the descriptor (read-only) and selected at compile time by `ADD_EVENT`. The 1-arg form (`ADD_EVENT(name)`) binds the default `evntHandler`, which wakes `event->stateMachine` whenever `evntType == trigger`. The 2-arg form (`ADD_EVENT(name, fn)`) binds a custom handler with signature `int (*)(volatile event_t *)`.
- **ISR contract.** ISRs call only `evntTrigger(event, subType)`. The handler runs from `evntDispatch()` in main-loop (cooperative) context, never in ISR context.
- **Self-arming pattern.** A system event whose handler runs directly from `evntDispatch` (no FSM is waiting) uses `evntArmSystem` to re-arm itself at the end of the handler. `evntDispatch` recognizes `event->stateMachine == NULL` as a system event and skips its auto-disarm-and-sister-scan path so the re-arm is preserved. Reference implementation: `sysUpdateWaitTicks()` in [sys/sys.c](../sys/sys.c).
- **Wakeup semantics for FSM-bound events.** When an FSM-bound event's handler returns, `evntDispatch` also disarms any other events armed for the same `stateMachine`. This implements "wait on any one of N events" — only the first to trigger fires; the others are silently disarmed.

#### 4.3.4 Event Sub-Type Contract

`event_t` carries two integer fields that together implement the
sub-type contract:

| Field | Set by | When |
|-------|--------|------|
| `evntType` | `evntWait(sm, ev, type)` | When a consumer arms the event for the condition it wants to wait for. |
| `trigger` | `evntTrigger(ev, subType)` | When a producer raises the event (ISR or other state machine). |

The default handler `evntHandler()` releases the waiting state machine
only when `event->evntType == event->trigger`. That lets one event
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

### 4.5 High Resolution Timer

#### 4.5.1 Responsibilities

- Provides a mechanism for FSM delays with high precision
- Can delay an FSM from 1 microsecond to 1 hour and 11.5 minutes. With a resolution of 1 microsecond
- Uses events to put the FSM on the WAIT queue
- Provides event or interrupt call back handlers to note the event and possibly reschedule the FSM
- The call back function can use evntTrigger to reschedule the FSM
- The call back function can be in either the interrupt context for less jitter. Or, It can be in the event call back context which is in the system/FSM context therefore not requiring thread safety with the rest of the FSMs and events

#### 4.5.2 Data Structures

| Structure | Description |
|-----------|-------------|
| Timer_t   | RAM based collection of data describing the timer's state. This includes the original duration and remaining microseconds |
| TimerDescr_t | Flash based descritpion of the timer including it's name, a pointer to the Timer_t state in RAM, and a pointer to the associated Event |

---

### 4.5 File I/O Abstraction (fio)

**Files:** [sys/fio.h](../sys/fio.h)

#### 4.5.1 Responsibilities

- Bridge AVR-libc `FILE` streams to avrOS queues.
- Allow standard C I/O functions (`fprintf`, `fgetc`, etc.) to work with UART and other buffered peripherals.
- Provide blocking wait helpers that use the event system.

#### 4.5.2 Data Structures

| Structure | Description |
|-----------|-------------|
| `fioBuffers_t` | Holds pointers to input and output `queue_t` instances for a `FILE` stream. |

#### 4.5.3 Key Interfaces

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

---

### 6.4 DAC Driver (dac)

**Files:** [drv/dac.c](../drv/dac.c), [drv/dac.h](../drv/dac.h)

#### 6.4.1 Responsibilities

- Initialize the AVR-Dx 10-bit DAC peripheral with a configurable voltage reference.
- Provide a simple output function used by the PCM audio service.

#### 6.4.2 Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `DAC_MAX` | `0x03FF` | Maximum DAC output code (full scale) |
| `DAC_MID` | `0x01FF` | Midpoint DAC output code |
| `DAC_MIN` | `0x0000` | Minimum DAC output code (zero) |

#### 6.4.3 Key Interfaces

| Function | Description |
|----------|-------------|
| `void dacInit(VREF_REFSEL_t vRef, register16_t output)` | Initialize the DAC with the given voltage reference and initial output value. |
| `void dacOutput(int value)` | Write a sample value to the DAC output register. |

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
// in an FSM state:
evntWait(stateMachine, evntGetEvent("Button_event"), GPIO_EVENT_FALLING);
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

---

## 10. Revision History

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0 | 2026-02-24 | John Anderson | Initial outline |
| 1.1 | 2026-05-17 | (consolidation) | Add §3.4 Memory Layout, §3.5 Linker Sections, expanded §4.2 priority detail, §4.3.4 event sub-type contract, §9 Verification & Testing. |
