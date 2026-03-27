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
9. [Revision History](#9-revision-history)

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

1. Wake from idle on a timer tick interrupt.
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

- The tick ISR fires the `EVENT_TYPE_TICK` event, waking any FSM waiting on the system tick.
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

| Constant | Value | Description |
|----------|-------|-------------|
| `FSM_DRV` | `0x00` | Driver priority (highest) |
| `FSM_SYS` | `0x40` | System priority |
| `FSM_SRV` | `0x80` | Service priority |
| `FSM_APP` | `0xC0` | Application priority (lowest) |

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

- Provide a mechanism for FSMs to wait on more than one hardware or software conditions.
- Allow ISRs and other FSMs to signal events that wake waiting state machines.

#### 4.3.2 Key Interfaces

| Function / Macro | Description |
|-----------------|-------------|
| `evntWait(event, condition)` | Suspend the current FSM until `condition` is met on `event`. |
| `evntTrigger(event, condition)` | Signal an event condition, resuming all waiting FSMs. |

#### 4.3.3 Design Notes

- `EVENT_TYPE_TICK` is predefined for system tick synchronization (see `sys.h`).
- Queue events (`QUE_EVENT_NOT_EMPTY`, `QUE_EVENT_EMPTY`) are used by the FIO layer.
- A linked list of events is maintained for each state machine's `fsmStateMachine_t` data structure
- `evntWait()` adds a new event to the state machine's data structure and puts the state machine in the wait queue if it's not already there.
- `evntTrigger()` scans the wait queue for all state machines waiting on this event. For each state machine it finds, it clears the entire linked list of events and puts the state machine back on the ready queue in priority order.

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
- Fire an optional callback handler on pin change (interrupt-driven).
- Collect optional toggle statistics per instance.

#### 6.5.2 Data Structures

| Structure | Description |
|-----------|-------------|
| `gpio_t` | Compile-time GPIO descriptor: PORT pointer, pin bit mask, direction, callback handler, optional stats pointer. |
| `gpioStats_t` | Toggle counter (conditionally compiled with `GPIO_STATS`). |

#### 6.5.3 Pin and Direction Types

| Type | Values |
|------|--------|
| `gpioPin_t` | `GPIO_PIN_0` … `GPIO_PIN_7` (individual pin bit masks) |
| `gpioDirection_t` | `GPIO_OUTPUT`, `GPIO_INPUT` |

#### 6.5.4 Registration Macro

| Macro | Description |
|-------|-------------|
| `ADD_GPIO(name, port, pin, dir, ...)` | Declare and register a GPIO instance; optional callback handler as last argument. |

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

## 9. Revision History

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0 | 2026-02-24 | John Anderson | Initial outline |
