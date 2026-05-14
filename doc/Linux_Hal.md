# avrOS Linux HAL — Implementation Plan

This document is the detailed implementation plan for running avrOS under a native Linux
GCC toolchain for unit testing, integration testing, memory leak detection, and coverage
analysis. It follows the approach proposed in the initial architecture review and is
organized as eight sequential phases, each with a concrete AI prompt that can be given
directly to a coding assistant to implement that phase.

---

## Background and Design Constraints

### Why This Approach Works

avrOS uses two mechanisms to register objects at compile time:

1. **Linker section macros** — `SECTION(FSM_TABLE)`, `SECTION(CLI_CMDS)`, etc. place
   descriptors into named ELF sections. GNU ld on Linux generates `__start_SECTION` and
   `__stop_SECTION` symbols for any named section that contains at least one object, which
   is exactly what the drivers iterate at runtime. This mechanism **works identically on
   Linux ELF** without modification.

2. **AVR-libc types and peripheral registers** — `USART_t`, `PORT_t`, `TCB_t`, and the
   hundreds of `_gc`/`_bm`/`_bp` constants are defined in the Device Family Pack headers
   (`<avr/io.h>`, etc.) that are only available when cross-compiling for AVR. These must
   be replaced with stub headers on Linux.

### Core Principle

**No existing source file is modified.** All AVR-specific symbols are provided by a set
of stub headers placed in `test/hal/` and activated by adding `-Itest/hal` *before* the
system include path on the Linux build. The `-DLINUX_HAL` preprocessor flag gates any
Linux-specific logic inside the HAL and Linux driver files.

### Directory Structure

```
avrOS/
  test/
    hal/                        Phase 1 — AVR header stubs
      avr/
        io.h                    Peripheral struct + all _gc/_bm/_bp constants
        interrupt.h             ISR() macro, sei(), cli()
        pgmspace.h              PROGMEM, PSTR, pgm_read_*
        cpufunc.h               ccp_write_io()
        sleep.h                 sleep_mode(), set_sleep_mode()
      util/
        atomic.h                ATOMIC_BLOCK
      linux_hal.h               Phase 2 — Peripheral instance declarations
      linux_hal.c               Phase 2 — Peripheral instance definitions

    linux_drv/                  Phase 3 — Linux-native driver replacements
      cpu.c                     cpuGetFrequency(), cpuSetOSCHF(), cpuReset()
      sys_tick.c                sysInitTick() via POSIX timer_create
      uart.c                    UART backed by POSIX pipe pairs
      gpio.c                    GPIO no-op stubs + ISR stubs
      dac.c                     DAC no-op stubs
      mem.c                     Linux /proc-based RAM stats + no-op stack fill

    fio/                        Phase 4 — FIO/FILE compatibility layer
      fio_linux.h               Linux-compatible FILE struct + fio macros
      fio_linux.c               uartPutChar/uartGetChar for pipe-backed FILE

    unity/                      Phase 5 — Unity test framework
      unity.h
      unity.c
      unity_internals.h

    unit/                       Phase 6 — Unit tests
      test_queue.c
      test_event.c
      test_fsm.c
      makefile

    integration/                Phase 7 — Integration tests
      test_cli_integration.c
      test_fsm_integration.c
      makefile

    makefile                    Phase 8 — Top-level test makefile
```

---

## Phase 1 — AVR Header Stubs

### Objective

Create `test/hal/avr/io.h` and companion headers so that every source file in `sys/`,
`drv/`, and `srv/` compiles under `gcc` without modification. These headers shadow the
real AVR-libc headers via compiler `-I` path ordering.

### Files to Create

| File | Contents |
|---|---|
| `test/hal/avr/io.h` | All peripheral register struct definitions (`USART_t`, `PORT_t`, `TCB_t`, `DAC_t`, `VREF_t`, `CLKCTRL_t`, `RSTCTRL_t`); all `_gc`/`_bm`/`_bp`/`_gm` constants used in the source tree; AVR memory constants (`RAMEND`, `RAMSIZE`, `PROGMEM_SIZE`, `MAPPED_PROGMEM_SIZE`); `FUSES` and `LOCKBITS` as no-op macros |
| `test/hal/avr/interrupt.h` | `ISR(vector)` expands to `void vector##_handler(void)`; `sei()` and `cli()` expand to `((void)0)` |
| `test/hal/avr/pgmspace.h` | `PROGMEM` expands to nothing; `PSTR(s)` expands to `(s)`; `pgm_read_byte(p)` expands to `(*(uint8_t*)(p))`; `pgm_read_word(p)` expands to `(*(uint16_t*)(p))`; `ROM_STR`/`ROM_STR_G` expand to plain `static const char[]` |
| `test/hal/avr/cpufunc.h` | `ccp_write_io(addr, val)` expands to `(*(volatile uint8_t*)(addr) = (val))` |
| `test/hal/avr/sleep.h` | `set_sleep_mode(m)` and `sleep_mode()` expand to `((void)0)` |
| `test/hal/util/atomic.h` | `ATOMIC_RESTORESTATE` and `ATOMIC_FORCEON` are empty; `ATOMIC_BLOCK(type)` expands to `for(int _ab=1; _ab; _ab=0)` |

### Peripheral Struct Details for `avr/io.h`

```c
// Required by uart.c, uart.h
typedef struct {
    uint16_t BAUD;
    uint8_t  RXDATAL, RXDATAH, TXDATAL, TXDATAH;
    uint8_t  STATUS, CTRLA, CTRLB, CTRLC, CTRLD;
    uint8_t  DBGCTRL, EVCTRL, TXPLCTRL, RXPLCTRL;
} USART_t;
typedef USART_t  USART_PMODE_t;   // enum aliases used in UART_t fields
typedef uint8_t  USART_CHSIZE_t;
typedef uint8_t  USART_SBMODE_t;

// Required by gpio.c, gpio.h
typedef struct {
    uint8_t DIR, DIRSET, DIRCLR, DIRTGL;
    uint8_t OUT, OUTSET, OUTCLR, OUTTGL;
    uint8_t IN, INTFLAGS, PORTCTRL, _pad[5];
    uint8_t PIN0CTRL, PIN1CTRL, PIN2CTRL, PIN3CTRL,
            PIN4CTRL, PIN5CTRL, PIN6CTRL, PIN7CTRL;
} PORT_t;

// Required by sys.c
typedef struct {
    uint8_t  CTRLA, CTRLB;
    uint16_t CCMP, CNT;
    uint8_t  INTCTRL, INTFLAGS, EVCTRL, DBGCTRL, TEMP;
} TCB_t;

// Required by dac.c
typedef struct { uint8_t CTRLA; uint16_t DATA; } DAC_t;
typedef struct { uint8_t DAC0REF; } VREF_t;

// Required by cpu.c
typedef uint8_t CLKCTRL_FRQSEL_t;
typedef uint8_t CLKCTRL_PDIV_t;
typedef uint8_t CLKCTRL_CLKSEL_t;
typedef struct {
    uint8_t MCLKCTRLA, MCLKCTRLB, MCLKLOCK, MCLKSTATUS;
    uint8_t _pad[12];
    uint8_t OSCHFCTRLA;
} CLKCTRL_t;
typedef struct { uint8_t SWRR; } RSTCTRL_t;

// External instances declared here; defined in linux_hal.c
extern PORT_t   PORTA, PORTC, PORTD, PORTF;
extern USART_t  USART0, USART1, USART2;
extern TCB_t    TCB0, TCB1, TCB2;
extern DAC_t    DAC0;
extern VREF_t   VREF;
extern CLKCTRL_t CLKCTRL;
extern RSTCTRL_t RSTCTRL;
```

All `_gc`/`_bm`/`_bp` constants must be provided as `#define` literals (not enums) so
they match the types expected by the existing source — scan `grep -rh '_gc\|_bm\|_bp\|_gm'
sys/ drv/ srv/` to enumerate every constant that needs a definition.

### Known AVR Memory Constants

```c
// Stub values — sized to match AVR128DA28 for realistic mem reporting
#define RAMEND              0x3FFF
#define RAMSIZE             0x4000
#define PROGMEM_SIZE        0x20000UL
#define MAPPED_PROGMEM_SIZE 0x8000
```

### `main.c`-Specific Macros

`main.c` uses `FUSES = { ... }` and `LOCKBITS = ...` which are avr-libc section macros.
Define them as no-ops so `main.c` compiles on Linux:

```c
#define FUSES       static const __attribute__((unused)) struct _fuses
#define LOCKBITS    static const __attribute__((unused)) uint8_t _lockbits __attribute__((unused)) =
```

---

### Phase 1 AI Prompt

> You are implementing a Linux HAL shim for an AVR embedded OS called avrOS so its core
> modules (`sys/`, `drv/`, `srv/`) can be compiled and tested natively on Linux with GCC.
> No existing source file may be modified. All changes live under `test/hal/`.
>
> **Task:** Create the following six header files exactly as specified.
>
> **`test/hal/avr/io.h`**
> - Include guard `AVR_IO_H_STUB`.
> - Include `<stdint.h>` and `<stdbool.h>`.
> - Define `register16_t` as `uint16_t`.
> - Define these peripheral register structs exactly matching the field names used in the
>   avrOS source tree: `USART_t`, `PORT_t`, `TCB_t`, `DAC_t`, `VREF_t`, `CLKCTRL_t`,
>   `RSTCTRL_t`. (Struct field lists are given in the plan above.)
> - Define typedef aliases `USART_PMODE_t`, `USART_CHSIZE_t`, `USART_SBMODE_t` as
>   `uint8_t`.
> - Define typedef aliases `CLKCTRL_FRQSEL_t`, `CLKCTRL_PDIV_t`, `CLKCTRL_CLKSEL_t` as
>   `uint8_t`.
> - Declare external instances: `PORTA`, `PORTC`, `PORTD`, `PORTF` (PORT_t); `USART0`,
>   `USART1`, `USART2` (USART_t); `TCB0`, `TCB1`, `TCB2` (TCB_t); `DAC0` (DAC_t);
>   `VREF` (VREF_t); `CLKCTRL` (CLKCTRL_t); `RSTCTRL` (RSTCTRL_t).
> - Scan every `_gc`, `_bm`, `_bp`, `_gm` usage in `sys/*.c`, `drv/*.c`, `srv/*.c`,
>   `drv/*.h`, `srv/*.h`, `sys/*.h`, `app/avrOS_example/avrOSConfig.h` and define every
>   constant as a `#define` with the correct integer literal. Group by peripheral
>   (USART_*, TCB_*, PORT_*, CLKCTRL_*, DAC_*, VREF_*, RSTCTRL_*).
> - Add `RAMEND`, `RAMSIZE`, `PROGMEM_SIZE`, `MAPPED_PROGMEM_SIZE` as given above.
> - Add `FUSES` and `LOCKBITS` no-op macros as given above.
> - Add a `SYS_TIMER_TCB0`, `SYS_TIMER_TCB1`, `SYS_TIMER_TCB2` definition that matches
>   what `sys/sys.h` uses for the `SYS_TICK_TIMER` comparison (cast to pointer or
>   address of TCB instance).
>
> **`test/hal/avr/interrupt.h`**
> - Include guard `AVR_INTERRUPT_H_STUB`.
> - `#define ISR(vector)  void vector##_handler(void)`
> - `#define sei()  ((void)0)`
> - `#define cli()  ((void)0)`
>
> **`test/hal/avr/pgmspace.h`**
> - Include guard `AVR_PGMSPACE_H_STUB`.
> - `#define PROGMEM` (empty)
> - `#define PSTR(s)  (s)`
> - `#define pgm_read_byte(p)   (*(const uint8_t *)(p))`
> - `#define pgm_read_word(p)   (*(const uint16_t *)(p))`
> - `#define pgm_read_dword(p)  (*(const uint32_t *)(p))`
> - `#define ROM_STR(var,str)   static const char var[] = {str}`
> - `#define ROM_STR_G(var,str) const char var[] = {str}`
>
> **`test/hal/avr/cpufunc.h`**
> - Include guard `AVR_CPUFUNC_H_STUB`.
> - `#define ccp_write_io(addr, val)  (*(volatile uint8_t *)(addr) = (uint8_t)(val))`
>
> **`test/hal/avr/sleep.h`**
> - Include guard `AVR_SLEEP_H_STUB`.
> - `#define set_sleep_mode(m)  ((void)0)`
> - `#define sleep_mode()       ((void)0)`
> - `#define sleep_enable()     ((void)0)`
> - `#define sleep_disable()    ((void)0)`
>
> **`test/hal/util/atomic.h`**
> - Include guard `UTIL_ATOMIC_H_STUB`.
> - `#define ATOMIC_RESTORESTATE  /* empty */`
> - `#define ATOMIC_FORCEON       /* empty */`
> - `#define ATOMIC_BLOCK(type)   for(int _atomic_once=1; _atomic_once; _atomic_once=0)`
>
> After creating the files, verify by running:
> ```
> gcc -std=gnu99 -Itest/hal -I. -DLINUX_HAL -DFSM_STATS -DQUE_STATS -DEVNT_STATS \
>     -DUART_STATS -DGPIO_STATS -DDEBUG -DCLI \
>     -Iapp/avrOS_example -c sys/queue.c sys/event.c sys/fsm.c
> ```
> and confirm zero errors.

---

## Phase 2 — Peripheral Instance Definitions and Linux HAL Header

### Objective

Provide the concrete `extern` instances declared in `avr/io.h` and a `linux_hal.h`
header that declares the Linux-side functions (tick ISR type, pipe-backed UART helpers)
used by later phases.

### Files to Create

**`test/hal/linux_hal.c`** — defines every peripheral register struct instance as a
zero-initialized global:

```c
#include "linux_hal.h"
PORT_t   PORTA, PORTC, PORTD, PORTF;
USART_t  USART0, USART1, USART2;
TCB_t    TCB0, TCB1, TCB2;
DAC_t    DAC0;
VREF_t   VREF;
CLKCTRL_t CLKCTRL;
RSTCTRL_t RSTCTRL;
```

**`test/hal/linux_hal.h`** — declares:
- All peripheral instances (re-exported from `avr/io.h` via `linux_hal.h` include chain).
- A function prototype `void linux_hal_tick_isr(void)` which the sys_tick driver calls.
- A function prototype `void linux_hal_init(void)` to zero-initialize instances.

### Phase 2 AI Prompt

> You are continuing the Linux HAL shim for avrOS. Phase 1 created the AVR header stubs
> under `test/hal/avr/` and `test/hal/util/`. Phase 2 requires two files.
>
> **Task:** Create `test/hal/linux_hal.h` and `test/hal/linux_hal.c`.
>
> `linux_hal.h`:
> - Include guard `LINUX_HAL_H`.
> - Include `"avr/io.h"` (the stub).
> - Re-declare (as `extern`) all peripheral instances: `PORTA`, `PORTC`, `PORTD`,
>   `PORTF`, `USART0`, `USART1`, `USART2`, `TCB0`, `TCB1`, `TCB2`, `DAC0`, `VREF`,
>   `CLKCTRL`, `RSTCTRL`.
> - Declare `void linux_hal_init(void)` — zeros all peripheral instances.
> - Declare `void linux_hal_tick_isr(void)` — defined in `linux_drv/sys_tick.c`; calls
>   the tick ISR body function emitted by the `ISR()` stub macro.
>
> `linux_hal.c`:
> - Include `"linux_hal.h"`.
> - Provide **definitions** (not just declarations) of all peripheral instances:
>   `PORT_t PORTA, PORTC, PORTD, PORTF;` etc. — all zero-initialized.
> - Implement `linux_hal_init()` using `memset` to zero each instance.
>
> After creating these, verify the full compile step from Phase 1 still passes (add
> `test/hal/linux_hal.c` to the command line).

---

## Phase 3 — Linux-Native Driver Replacements

### Objective

Replace the six hardware-dependent driver/system files (`drv/cpu.c`, `drv/uart.c`,
`drv/gpio.c`, `drv/dac.c`, `drv/mem.c`, `sys/sys.c`) with Linux-native equivalents
that satisfy the same public API. The original files are *not* compiled in test builds;
these replacements are compiled instead.

### File-by-File Specification

#### `test/linux_drv/cpu.c`
The only functions needed are `cpuGetFrequency()`, `cpuSetOSCHF()`, `cpuClockOut()`,
and `cpuReset()`. On Linux:
- `cpuGetFrequency()` returns the constant `24000` (matching `CPU_SPEED` = 24 MHz,
  in kHz units matching the AVR convention used by `sys.c`).
- `cpuSetOSCHF()`, `cpuClockOut()` are no-ops that return immediately.
- `cpuReset()` calls `exit(0)` (safe for test; avoids needing `ccp_write_io`).
- All CLI commands (`CPU_CLI`) are still compiled — they call `printf` which works.

#### `test/linux_drv/sys_tick.c`
Replaces the `ISR(SYS_TICK_INT_VECT)` and `sysInitTick()` from `sys/sys.c`. On Linux:
- `sysInitTick(TCB_t *tcb, uint16_t sysTickFreq)` creates a POSIX real-time timer using
  `timer_create(CLOCK_MONOTONIC, ...)` with `SIGEV_THREAD` notification.
- The notification callback calls `TCB0_INT_vect_handler()` — the function produced by
  the `ISR(TCB0_INT_vect)` stub in `sys/sys.c`.
- The interval is `1 000 000 000 / sysTickFreq` nanoseconds.
- Link with `-lrt -lpthread`.

Note: `sys.c` itself is still compiled — only its `sysInitTick()` and `ISR` body are
replaced. Because `sysInitTick()` is a non-static internal function defined in `sys.c`,
this replacement must be compiled as a separate translation unit that is linked *in place*
of the version in `sys.c`. The simplest approach is to `#define sysInitTick
linux_sysInitTick` via a compile flag, or to guard the function in `sys.c` with
`#ifndef LINUX_HAL`. Since we cannot modify `sys.c`, use a linker `--wrap` or compile
`sys.c` without the conflicting function by providing it in this file with the same name
and relying on link order (the Linux driver object is listed first in the makefile OBJ
list so it wins at link time — GNU ld uses first-definition-wins for non-weak symbols is
**incorrect**; instead, mark the replacement as a strong symbol and the original as weak
using `__attribute__((weak))` added via a `-D` macro: add
`-DsysInitTick=__weak_sysInitTick` to the `sys.c` compile flags only, and define the
Linux version as the strong `sysInitTick`). See the Phase 3 makefile fragment below.

#### `test/linux_drv/uart.c`
Provides `uartInit()` and `uartPutChar()`/`uartGetChar()` backed by POSIX pipe pairs.
- Each `UART_t` instance has two pipe file descriptors (RX pipe: test writes, UART reads;
  TX pipe: UART writes, test reads). Store these in a side-table array keyed by
  `USART_t *` pointer since the `UART_t` struct cannot be modified.
- `uartInit()`: opens a pipe pair for the instance, stores FDs, sets up the interrupt
  (DRE/RXC) simulation — replaced by direct queue operations on Linux. Specifically, on
  Linux `uartInit()` starts a background `pthread` reader thread that calls
  `quePutByte(uart->rxQueue, byte)` for each byte that arrives on the RX pipe.
- `uartPutChar(char c, FILE *f)`: calls `queGet(txQueue)` and writes to the TX pipe FD.
  Used by the `FILE.put` function pointer registered in `ADD_UART_RW`.
- `uartGetChar(FILE *f)`: calls `queGet(rxQueue)` if non-empty; returns `EOF` otherwise.
- `uartName(UART_t *uart)`: returns the `uart->name` field (already in `UART_t`).
- All UART ISR stub bodies (`USART0_DRE_vect_handler`, etc.) are defined here as
  no-ops since the pipe-thread model replaces them.

#### `test/linux_drv/gpio.c`
- All GPIO ISR stub bodies (`PORTA_PORT_vect_handler`, etc.) are empty functions.
- `gpioInit()`, `gpioSetOutput()`, `gpioClearOutput()`, `gpioToggleOutput()`,
  `gpioWriteOutput()`, `gpioReadOutput()`, `gpioReadInput()` operate on the
  `gpio->port->OUT` / `gpio->port->IN` fields of the stub `PORT_t` struct — these are
  real memory locations (since `linux_hal.c` defines the PORT instances as globals) so
  the logic works correctly for unit test assertions.

#### `test/linux_drv/dac.c`
- `dacInit()` and `dacOutput()` are no-ops. The stub `DAC0.DATA` field (in `linux_hal.c`)
  holds whatever `dacOutput()` would write — useful for assertion in tests.

#### `test/linux_drv/mem.c`
Replaces `drv/mem.c`. On Linux:
- `memStackFill()` — no-op.
- `memStackSizeMax()` — returns `0`.
- `memRamStatus(FILE *f)` / `memRomStatus(FILE *f)` — read `/proc/self/status` for
  `VmRSS` and `VmSize`; print a simplified summary. These can degrade gracefully if
  `/proc` is unavailable.
- Inline functions in `drv/mem.h` (`memRamSize()`, `memDataSize()`, etc.) reference AVR
  linker symbols (`__data_start`, `__heap_start`, `__brkval`, `_etext`, etc.) that do not
  exist on Linux. Add a stub header `test/hal/linux_mem_syms.h` that provides these as
  references to ordinary globals initialized to sensible constants, and include it from
  `test/hal/avr/io.h` when `LINUX_HAL` is defined.

### Phase 3 AI Prompt

> You are implementing Phase 3 of the avrOS Linux HAL. Phases 1 and 2 provide AVR header
> stubs and peripheral instance globals. Phase 3 replaces the six hardware-dependent
> driver/system source files with Linux-native equivalents compiled instead of the
> originals. **Do not modify any file outside `test/`.**
>
> **Task:** Create the following six files. For each, include `"../avrOS.h"` using the
> `test/hal/` include path so the AVR stubs apply.
>
> **`test/linux_drv/cpu.c`**
> - Include `"../../avrOS.h"` (path adjusted for build context).
> - `uint16_t cpuGetFrequency()` — returns `24000`.
> - `void cpuSetOSCHF(CLKCTRL_FRQSEL_t f, bool pe, CLKCTRL_PDIV_t p)` — no-op.
> - `void cpuClockOut(bool enable)` — no-op.
> - `void cpuReset()` — calls `exit(0)`.
> - Reproduce the `#ifdef CPU_CLI` / `ADD_COMMAND` block verbatim from `drv/cpu.c` so
>   the CLI command is registered.
>
> **`test/linux_drv/sys_tick.c`**
> - Include `<time.h>`, `<signal.h>`, `<string.h>`.
> - Declare `extern void TCB0_INT_vect_handler(void)` (produced by the ISR stub in
>   `sys/sys.c`).
> - Static POSIX `timer_t tickTimer`.
> - `static void tick_notify(union sigval sv)` — calls `TCB0_INT_vect_handler()`.
> - `void sysInitTick(TCB_t *tcb, uint16_t sysTickFreq)`:
>   - Creates `SIGEV_THREAD` timer on `CLOCK_MONOTONIC`.
>   - Sets interval to `1 000 000 000L / sysTickFreq` ns.
>   - Also calls `evntEnable(&tick, EVENT_TYPE_TICK, sysUpdateWaitTicks, NULL)`.
>     (The `tick` event and `sysUpdateWaitTicks` are declared in `sys/sys.c`; expose them
>     via `extern` declarations here since they are non-static.)
> - This file provides the strong definition of `sysInitTick` so the linker discards the
>   version in `sys.c`. The makefile must compile `sys.c` with
>   `-DsysInitTick=sysInitTick_avr` to rename the AVR version at compile time only.
>
> **`test/linux_drv/uart.c`**
> - Include `<unistd.h>`, `<pthread.h>`, `<fcntl.h>`.
> - Define a side-table `struct UartPipes { int rxfd, txfd; pthread_t reader; }` and an
>   array of 3 entries (one per USART instance).
> - `int uartInit(const fsmStateMachineDescr_t *descr)`:
>   - Gets `UART_t *uart = initGetInstance(descr)`.
>   - Opens a pipe pair for RX (test→uart) and TX (uart→test).
>   - Spawns a reader thread that loops: `read(rxfd, &b, 1)` → `quePutByte(uart->rxQueue,
>     b)` → signals `queGetEvent(uart->rxQueue)`.
>   - Stores `txfd` so `uartPutChar` can write to it.
> - `int uartPutChar(char c, FILE *f)`:
>   - Gets `UART_t *uart = (UART_t *)f->udata`.
>   - Gets the pipe index; writes `c` to the TX pipe FD.
>   - Returns `0`.
> - `int uartGetChar(FILE *f)`:
>   - Gets `UART_t *uart = (UART_t *)f->udata`.
>   - Calls `queGetByte(uart->rxQueue, &b)`; returns `b` or `EOF`.
> - `char *uartName(UART_t *uart)` — returns a string from the USART pointer
>   (`USART0`→"USART0", etc.).
> - Define empty ISR stub bodies: `void USART0_DRE_vect_handler(void) {}`, etc.
>   (6 functions for the 3 DRE + 3 RXC vectors declared by the interrupt stub).
> - Expose two test-helper functions:
>   `int uartTestGetTxFd(UART_t *uart)` and `int uartTestGetRxFd(UART_t *uart)`
>   so integration tests can inject/read characters without accessing internals directly.
>
> **`test/linux_drv/gpio.c`**
> - Define empty ISR stubs: `PORTA_PORT_vect_handler`, `PORTC_PORT_vect_handler`,
>   `PORTD_PORT_vect_handler`, `PORTF_PORT_vect_handler`.
> - Implement `gpioInit(const fsmStateMachineDescr_t *descr)` using the existing logic
>   from `drv/gpio.c` but replacing `PORT_t` register writes with direct writes to the
>   stub struct fields (this is safe since the PORT instances are real globals).
> - Implement all `gpioSet/Clear/Toggle/Write/ReadOutput/ReadInput` functions to
>   operate on `gpio->port->OUT` and `gpio->port->IN` directly.
> - Copy the `#ifdef GPIO_CLI` block verbatim from `drv/gpio.c`.
>
> **`test/linux_drv/dac.c`**
> - `void dacInit(VREF_REFSEL_t vRef, register16_t output)` — assigns `DAC0.DATA =
>   output` and returns.
> - `void dacOutput(int16_t value)` — clamps and assigns `DAC0.DATA` as in the original.
>
> **`test/linux_drv/mem.c`**
> - `void memStackFill()` — no-op.
> - `uint16_t memStackSizeMax()` — returns `0`.
> - `void memRamStatus(FILE *f)` — reads `/proc/self/status` (if available) for `VmRSS`
>   and `VmPeak`; prints two lines. If `/proc/self/status` is unavailable, prints
>   `"RAM status unavailable on this platform\n"`.
> - `void memRomStatus(FILE *f)` — prints `"ROM status not applicable on Linux\n"`.
> - Reproduce the `#ifdef MEM_CLI` / `ADD_COMMAND` block so the CLI commands compile.
>
> Also create `test/hal/linux_mem_syms.h`:
> - Declares/defines the AVR linker symbols as plain C globals so that the inline
>   functions in `drv/mem.h` link on Linux:
>   `extern uint16_t __data_start, __data_end, __heap_start; extern uint16_t *__brkval;`
>   `extern uint16_t _etext, __start_text_window, __stop_text_window, __stop_rodata;`
>   and in `linux_hal.c` provides zero-initialised definitions for all of them.
> - This file must be `#include`-d from `drv/mem.h` when `LINUX_HAL` is defined (add
>   a conditional `#ifdef LINUX_HAL / #include "linux_mem_syms.h" / #endif` at the top
>   of `drv/mem.h`). This is the **only permitted modification** to an existing source
>   file, and only if the guard prevents any impact on the AVR build.

---

## Phase 4 — FIO / FILE Compatibility Layer

### Objective

`sys/fio.h` accesses `file->buf`, `file->put`, `file->get`, `file->flags`, and
`file->udata` — fields of avr-libc's internal `FILE` struct that do not exist in glibc's
`FILE`. On Linux the `FILE` type from `<stdio.h>` is opaque. The solution is to define a
**parallel custom FILE type** used only when `LINUX_HAL` is defined, leaving all
`srv/cli.c`, `srv/log.c`, `drv/uart.h` source files unmodified.

### Design

Define a `typedef struct AVROS_FILE` in `test/fio/fio_linux.h` that mirrors the avr-libc
`FILE` layout exactly. When `LINUX_HAL` is defined, `avrOS.h` (via a conditional include)
replaces `#include <stdio.h>` with `#include "test/fio/fio_linux.h"` which provides this
custom `FILE` and stubs for `printf`, `fprintf`, `fgetc`, `fputc`, and `fgets` that route
through the queue-backed streams. Standard `printf` output is redirected to the Linux
process `stdout` for test visibility.

### Custom FILE Struct

```c
// test/fio/fio_linux.h
typedef struct AVROS_FILE {
    char   *buf;         // points to fioBuffers_t in UART macros
    int   (*put)(char, struct AVROS_FILE *);
    int   (*get)(struct AVROS_FILE *);
    uint8_t flags;
    void   *udata;       // points to UART_t instance
} FILE;

#define _FDEV_SETUP_READ   0x01
#define _FDEV_SETUP_WRITE  0x02
#define _FDEV_SETUP_RW     0x03

// Route printf/fprintf through the active FILE streams
extern FILE *stdout, *stdin, *stderr;
int avros_printf(const char *fmt, ...);
int avros_fprintf(FILE *f, const char *fmt, ...);
int avros_fgetc(FILE *f);
int avros_fputc(int c, FILE *f);
#define printf    avros_printf
#define fprintf   avros_fprintf
#define fgetc     avros_fgetc
#define fputc     avros_fputc
```

`avros_printf` writes to the `stdout` FILE via its `put` function pointer (which calls
`uartPutChar` routed to the TX pipe). Additionally it duplicates the output to the real
Linux `stdout` FD (fd 1) via `write(1, ...)` for test runner visibility.

### Phase 4 AI Prompt

> You are implementing Phase 4 of the avrOS Linux HAL — the FIO/FILE compatibility layer.
> On AVR, the avr-libc `FILE` struct has fields `buf`, `put`, `get`, `flags`, and `udata`
> accessed directly by `sys/fio.h`, `srv/cli.c`, `srv/log.c`, and `drv/uart.h`. On Linux,
> glibc's `FILE` is opaque and these fields do not exist. **No existing source file may be
> modified** (exception: the `drv/mem.h` change permitted in Phase 3).
>
> **Task:** Create `test/fio/fio_linux.h` and `test/fio/fio_linux.c`.
>
> `fio_linux.h`:
> - Include guard `FIO_LINUX_H`.
> - Include `<stdint.h>`, `<stdarg.h>`, `<string.h>`, `<unistd.h>`.
> - **Do not include `<stdio.h>`** — this file *replaces* stdio for avrOS sources.
> - Define `struct AVROS_FILE` with fields: `char *buf`, `int (*put)(char, struct
>   AVROS_FILE *)`, `int (*get)(struct AVROS_FILE *)`, `uint8_t flags`, `void *udata`.
> - `typedef struct AVROS_FILE FILE;`
> - Define `_FDEV_SETUP_READ 0x01`, `_FDEV_SETUP_WRITE 0x02`, `_FDEV_SETUP_RW 0x03`.
> - Define `EOF (-1)`.
> - Declare `extern FILE *stdout, *stdin, *stderr;`
> - Declare `int avros_printf(const char *fmt, ...)` and `#define printf avros_printf`.
> - Declare `int avros_fprintf(FILE *f, const char *fmt, ...)` and `#define fprintf
>   avros_fprintf`.
> - Declare `int avros_fgetc(FILE *f)` and `#define fgetc avros_fgetc`.
> - Declare `int avros_fputc(int c, FILE *f)` and `#define fputc avros_fputc`.
> - Declare `int avros_fputs(const char *s, FILE *f)` and `#define fputs avros_fputs`.
> - Declare `char *avros_fgets(char *s, int n, FILE *f)` and `#define fgets avros_fgets`.
>
> `fio_linux.c`:
> - Include `"fio_linux.h"` and `<stdio.h>` (the real glibc one — included here, not in
>   the header, to avoid redefinition conflicts).
> - Define `FILE *stdout = NULL, *stdin = NULL, *stderr = NULL;`
> - `avros_fprintf(FILE *f, const char *fmt, ...)`: vsnprintf into a stack buffer; if `f`
>   is not NULL and `f->put` is not NULL, iterate the buffer calling `f->put(c, f)` for
>   each character; also write to real `stdout` (fd 1) via `write(1, buf, len)` for test
>   visibility. Return character count.
> - `avros_printf(const char *fmt, ...)`: delegate to `avros_fprintf(stdout, fmt, ...)`.
> - `avros_fgetc(FILE *f)`: if `f` and `f->get`, return `f->get(f)`; else return `EOF`.
> - `avros_fputc(int c, FILE *f)`: if `f` and `f->put`, call `f->put((char)c, f)`;
>   return `c`.
> - `avros_fputs(const char *s, FILE *f)`: iterate calling `avros_fputc`.
> - `avros_fgets(char *s, int n, FILE *f)`: read up to `n-1` chars via `avros_fgetc`,
>   stop on `\n` or `EOF`.
>
> The file `avrOS.h` must include `fio_linux.h` instead of `<stdio.h>` when
> `LINUX_HAL` is defined. Since we cannot modify `avrOS.h`, instead pass
> `-include test/fio/fio_linux.h` to GCC **before** any source file is parsed, and add
> `-D'_STDIO_H 1'` to prevent `<stdio.h>` from being processed again by any indirect
> include. Document this in the `test/makefile`.
>
> Also define a utility function `FILE *fio_linux_create_stream(void *fioBuffers)` that
> allocates (via `malloc`) a `FILE`, sets `buf = fioBuffers`, `put = uartPutChar`,
> `get = uartGetChar`, `flags = _FDEV_SETUP_RW`. This is used by integration tests to
> create a test-controlled FILE stream.

---

## Phase 5 — Unity Test Framework

### Objective

Install the Unity C unit test framework (v2.6 or later, MIT license) into `test/unity/`.
Unity is a single-file framework: `unity.c`, `unity.h`, `unity_internals.h`.

### Phase 5 AI Prompt

> You are setting up the Unity C unit test framework for the avrOS Linux test suite.
>
> **Task:**
> 1. Download the three Unity source files (`unity.c`, `unity.h`,
>    `unity_internals.h`) from the official repository at
>    `https://github.com/ThrowTheSwitch/Unity` (tag `v2.6.0` or later) and place them in
>    `test/unity/`.
> 2. Create `test/unity/README.md` containing: the version number, the URL, and the
>    license (MIT).
> 3. Verify Unity compiles cleanly: `gcc -c test/unity/unity.c -o /tmp/unity.o`
>
> Do not add any CMake or Ruby infrastructure — only the three C files are needed.

---

## Phase 6 — Unit Tests

### Objective

Write isolated unit tests for the three portable core modules: `sys/queue.c`,
`sys/event.c`, and `sys/fsm.c`. These modules have no hardware dependencies; they compile
and run on Linux with only the HAL stubs from Phases 1–2.

### Test Coverage Targets

| Module | Functions / Behaviors to Test |
|---|---|
| `queue` | Init state; `quePut`/`queGet` with single element; full queue overflow; empty queue underflow; wrap-around at capacity boundary; `queGetSize`; `queIsEmpty`/`queIsFull`; pointer variant `quePutPtr`/`queGetPtr` |
| `event` | `evntEnable` → state becomes `EVENT_ARMED`; `evntTrigger` → state becomes `EVENT_TRIGGERED`; handler callback invoked; `evntDisable` → state becomes `EVENT_DISARMED`; `evntWait` with correct filter passes; wrong filter does not fire; `evntReset` clears state |
| `fsm` | `fsmInit` populates Ready queue from FSM_TABLE; priority ordering (DRV before SYS before SRV before APP); `fsmDispatch` calls state handlers in order; `fsmSetNextState` transitions on next cycle; `fsmIsInitialCall` true only on first call; `fsmStop`/`fsmReady`/`fsmGetInstance`; scan cycle counter increments |

### Phase 6 AI Prompt

> You are writing unit tests for avrOS using the Unity framework. The test build compiles
> `sys/queue.c`, `sys/event.c`, and `sys/fsm.c` natively on Linux using the HAL stubs
> in `test/hal/`. No hardware drivers are compiled. Tests use `ADD_QUEUE`,
> `ADD_EVENT`, and `ADD_STATE_MACHINE` macros, which rely on ELF linker sections that work
> on Linux GCC.
>
> **Task:** Create three test files and a makefile.
>
> **`test/unit/test_queue.c`**
> - Include `"../unity/unity.h"` and `"../../avrOS.h"`.
> - Use `ADD_QUEUE(testQ, sizeof(uint8_t), 8)` to create an 8-byte queue at file scope.
> - `setUp()` resets the queue to empty state by writing to head/tail fields directly.
> - Tests (name each `test_<behavior>`):
>   - `test_queue_initially_empty` — `TEST_ASSERT_TRUE(queIsEmpty(&testQ))`.
>   - `test_put_get_single` — put 0xAB, get it back, assert equal, assert empty.
>   - `test_put_increments_size` — put 3 bytes, assert `queGetSize(&testQ) == 3`.
>   - `test_full_queue_overflow` — fill to capacity, put one more, assert it returns false,
>     assert `queGetSize` is still at capacity.
>   - `test_empty_queue_underflow` — get from empty queue, assert returns false.
>   - `test_wrap_around` — put capacity bytes, get half, put half more; verify correct
>     data order (FIFO).
>   - `test_ptr_put_get` — use `quePutPtr`/`queGetPtr` with a pointer value, assert roundtrip.
> - `main()` calls `UNITY_BEGIN()`, all test functions via `RUN_TEST()`, `UNITY_END()`.
>
> **`test/unit/test_event.c`**
> - Create a minimal FSM stub state function `int dummyState(volatile fsmStateMachine_t
>   *sm)` that returns 0.
> - Use `ADD_EVENT(testEvent)` at file scope.
> - Tests:
>   - `test_event_initial_state` — check `testEvent.type == EVENT_TYPE_NONE`.
>   - `test_enable_arms_event` — call `evntEnable`, assert return is `EVENT_ARMED`.
>   - `test_trigger_fires_handler` — register a handler that sets a flag; trigger; assert
>     flag set.
>   - `test_wrong_filter_does_not_fire` — enable with `EVENT_TYPE_1`, trigger with
>     `EVENT_TYPE_2`, assert handler not called.
>   - `test_disable_disarms` — enable then disable; assert `evntDisable` returns
>     `EVENT_DISARMED`.
>   - `test_reset_clears_state` — trigger, reset, assert `type == EVENT_TYPE_NONE`.
>
> **`test/unit/test_fsm.c`**
> - Define 4 state handler functions: `int sm1State(volatile fsmStateMachine_t *sm)` …
>   `sm4State`. Each increments a per-SM counter and returns 0.
> - Use `ADD_STATE_MACHINE` at file scope to register 4 FSMs at priorities `FSM_DRV`,
>   `FSM_SYS`, `FSM_SRV`, `FSM_APP`.
> - `setUp()` calls `fsmInit()`.
> - Tests:
>   - `test_fsm_init_populates_ready_queue` — call `fsmInit()`; assert `fsmScanCycle()`
>     is 0.
>   - `test_dispatch_calls_all_states` — call `fsmDispatch()` once; assert all 4
>     counters == 1.
>   - `test_priority_order` — log call order via a shared array; assert DRV called before
>     SYS before SRV before APP.
>   - `test_state_transition` — in `sm1State`, on first call set next state to a new
>     handler; after two dispatches assert the new handler was called.
>   - `test_initial_call_flag` — in a new state, assert `fsmIsInitialCall(sm)` is true on
>     first call and false on second.
>   - `test_scan_cycle_increments` — call `fsmDispatch()` 5 times; assert
>     `fsmScanCycle() == 5`.
>   - `test_fsmStop_moves_to_stopped` — call `fsmStop(sm)`, then `fsmDispatch()` twice;
>     assert the SM's handler is not called again.
>   - `test_fsmReady_resumes` — stop then restart via `fsmReady`; assert handler called on
>     next dispatch.
>
> **`test/unit/makefile`**
> Build all three tests independently. Example for `test_queue`:
> ```makefile
> CC      = gcc
> CFLAGS  = -Wall -std=gnu99 -g -DLINUX_HAL -DQUE_STATS -DEVNT_STATS -DFSM_STATS \
>           -DDEBUG -DCLI -Itest/hal -I. -Iapp/avrOS_example \
>           -fprofile-arcs -ftest-coverage
> LDFLAGS = -lrt -lpthread --coverage
>
> test_queue: test/unit/test_queue.c sys/queue.c sys/event.c test/hal/linux_hal.c \
>             test/unity/unity.c
>     $(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
>
> test_event: test/unit/test_event.c sys/event.c sys/queue.c test/hal/linux_hal.c \
>             test/unity/unity.c
>     $(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
>
> test_fsm: test/unit/test_fsm.c sys/fsm.c sys/event.c sys/queue.c \
>           test/hal/linux_hal.c test/unity/unity.c
>     $(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
>
> unit: test_queue test_event test_fsm
>     ./test_queue && ./test_event && ./test_fsm
> ```

---

## Phase 7 — Integration Tests

### Objective

Run the full avrOS kernel — `sysInit()`, `fsmDispatch()`, and the tick timer — inside a
Linux process and validate end-to-end behavior:

1. **FSM integration**: multiple registered state machines dispatch correctly across
   priorities, state transitions work, tick-based waiting resolves.
2. **CLI integration**: inject command strings into the CLI UART RX pipe, read the
   response from the TX pipe, assert correct output.

### Phase 7 AI Prompt

> You are writing integration tests for avrOS on Linux. The full OS is compiled: `sys/`,
> `srv/`, and the Linux driver replacements from `test/linux_drv/`. `sysInit()` starts the
> POSIX tick timer, and `fsmDispatch()` is called in a loop from a test thread.
>
> **Task:** Create two integration test files.
>
> **`test/integration/test_fsm_integration.c`**
> - Include `"../../avrOS.h"` with Linux HAL flags.
> - Register a test state machine with `ADD_STATE_MACHINE(TestFsm, testFsmInit, FSM_APP)`.
> - `testFsmInit` transitions to `testFsmRun`; `testFsmRun` increments a counter and
>   calls `fsmWaitTicks(sm, 10)` (wait 10 ticks).
> - `main()`:
>   1. Call `sysInit()`.
>   2. Enable global events: `sei()`.
>   3. Run `fsmDispatch()` in a loop for 200 ms (using `clock_gettime` to bound time).
>   4. Assert `counter >= 5` (at 1 kHz tick with 10-tick wait, ~20 cycles in 200 ms).
>   5. Assert `fsmScanCycle() > 0`.
>   6. Print PASS/FAIL and return 0 or 1.
>
> **`test/integration/test_cli_integration.c`**
> - Call `sysInit()` to initialise all drivers and the CLI FSM.
> - Get the CLI UART's RX pipe FD via `uartTestGetRxFd(cliUart_ptr)` and TX pipe FD via
>   `uartTestGetTxFd(cliUart_ptr)`.
> - Test 1 — `help` command:
>   - Write `"help\r"` to the RX FD.
>   - Run `fsmDispatch()` in a loop for 50 ms.
>   - Read from the TX FD; assert the response contains `"help"` and `"?"`.
> - Test 2 — unknown command:
>   - Write `"badcmd\r"` to the RX FD.
>   - Run dispatch loop; read TX FD; assert response contains an error indicator.
> - Test 3 — `tick` command:
>   - Write `"tick\r"` to the RX FD.
>   - Run dispatch loop for 100 ms; read TX FD; assert response contains `"Tick"`.
> - Print PASS/FAIL for each test; return 0 if all pass, 1 otherwise.
>
> **`test/integration/makefile`**
> ```makefile
> LINUX_DRV = test/linux_drv/cpu.c test/linux_drv/uart.c test/linux_drv/gpio.c \
>             test/linux_drv/dac.c test/linux_drv/mem.c test/linux_drv/sys_tick.c
> CORE      = sys/fsm.c sys/event.c sys/queue.c sys/sys.c
> SRV       = srv/cli.c srv/log.c
> HAL       = test/hal/linux_hal.c
> FIO       = test/fio/fio_linux.c
>
> CFLAGS    = -Wall -std=gnu99 -g -DLINUX_HAL -DQUE_STATS -DEVNT_STATS -DFSM_STATS \
>             -DUART_STATS -DGPIO_STATS -DDEBUG -DCLI \
>             -DsysInitTick=sysInitTick_avr \
>             -Itest/hal -I. -Iapp/avrOS_example \
>             -include test/fio/fio_linux.h \
>             -fprofile-arcs -ftest-coverage
> LDFLAGS   = -lrt -lpthread --coverage
>
> integration: test_fsm_integration test_cli_integration
>     ./test_fsm_integration && ./test_cli_integration
>
> test_fsm_integration: test/integration/test_fsm_integration.c \
>                       $(CORE) $(LINUX_DRV) $(HAL) $(FIO) app/avrOS_example/main.c
>     $(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
>
> test_cli_integration: test/integration/test_cli_integration.c \
>                       $(CORE) $(SRV) $(LINUX_DRV) $(HAL) $(FIO) app/avrOS_example/main.c
>     $(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
> ```

---

## Phase 8 — Top-Level Makefile, Coverage, and Memory Leak Detection

### Objective

Provide a single `test/makefile` that orchestrates all phases, generates an HTML coverage
report with `lcov`/`genhtml`, and runs all test binaries under `valgrind`.

### Tools Required

| Tool | Package (Debian/Ubuntu) | Purpose |
|---|---|---|
| `gcc` | `build-essential` | Compilation |
| `lcov` | `lcov` | Coverage data collection and HTML report |
| `genhtml` | included with `lcov` | HTML report rendering |
| `valgrind` | `valgrind` | Heap memory leak detection |
| `gcov` | `gcov` (included with gcc) | Per-file coverage data |

Install: `sudo apt install build-essential lcov valgrind`

### Phase 8 AI Prompt

> You are creating the top-level `test/makefile` for the avrOS Linux test suite. It must
> support four targets: `unit`, `integration`, `coverage`, and `memcheck`.
>
> **Task:** Create `test/makefile` with the following specification.
>
> **Variables:**
> - `CC = gcc`
> - `UNITY = unity/unity.c`
> - `HAL = hal/linux_hal.c`
> - `FIO = fio/fio_linux.c`
> - `LINUX_DRV` — list all 6 files from `test/linux_drv/`.
> - `CORE` — `../sys/fsm.c ../sys/event.c ../sys/queue.c ../sys/sys.c`
> - `SRV` — `../srv/cli.c ../srv/log.c`
> - `CFLAGS_COMMON` — all `-D` flags, `-I` flags, and `-include test/fio/fio_linux.h`.
>   Include `-DsysInitTick=sysInitTick_avr` here.
> - `CFLAGS_COVERAGE` — `$(CFLAGS_COMMON) -fprofile-arcs -ftest-coverage`
> - `LDFLAGS` — `-lrt -lpthread --coverage`
>
> **Target: `unit`**
> - Builds and runs `test_queue`, `test_event`, `test_fsm` using `CFLAGS_COVERAGE`.
> - Compile commands derived from `test/unit/makefile` in Phase 6.
> - Prints a summary line: `=== Unit Tests PASSED ===` or exits non-zero on failure.
>
> **Target: `integration`**
> - Builds and runs `test_fsm_integration`, `test_cli_integration`.
> - Compile commands derived from `test/integration/makefile` in Phase 7.
> - Prints `=== Integration Tests PASSED ===` or exits non-zero.
>
> **Target: `coverage`**
> - Depends on `unit integration`.
> - Runs `lcov --capture --directory . --output-file coverage.info`.
> - Filters out HAL stubs and Unity: `lcov --remove coverage.info '*/test/hal/*'
>   '*/test/unity/*' '*/test/fio/*' --output-file coverage_filtered.info`.
> - Runs `genhtml coverage_filtered.info --output-directory coverage_html`.
> - Prints the path to `coverage_html/index.html`.
>
> **Target: `memcheck`**
> - Depends on building all test binaries (no `--coverage` flags — use `CFLAGS_COMMON`).
> - For each test binary, runs:
>   `valgrind --leak-check=full --track-origins=yes --error-exitcode=1 ./binary`
> - Prints `=== Memory Check PASSED ===` if all exit 0.
>
> **Target: `clean`**
> - Removes all `.o`, `*.gcda`, `*.gcno`, `*.gcov` files.
> - Removes test binaries.
> - Removes `coverage.info`, `coverage_filtered.info`.
> - `rm -rf coverage_html/`.
>
> **Target: `all`** — depends on `unit integration coverage memcheck`.
>
> Also create `test/README.md` that documents:
> - Prerequisites (`sudo apt install build-essential lcov valgrind`).
> - How to run each make target.
> - How to open the coverage report.
> - How to interpret valgrind output.

---

## Phase 9 — Interactive Console Application

### Objective

Build avrOS as a runnable Linux console application where a user at a terminal interacts
with the CLI in real time — same registered commands, same ANSI color output, same
escape-sequence handling — running the unmodified `srv/cli.c` and `sys/fsm.c` source
files that run on the AVR.

### How It Works

The Phase 3 pipe-backed UART already separates the "wire" from the hardware. Phase 9
connects that wire to the Linux terminal by:

1. Putting the terminal into **raw mode** (no kernel echo, no line buffering, no signal
   keys) so the CLI's own echo and escape-sequence handling takes over — exactly as it
   does on the AVR UART.
2. Running a **stdin bridge thread** that reads raw bytes from Linux stdin and writes them
   into the CLI UART's RX pipe, which the UART reader thread already drains into the RX
   queue.
3. Running a **TX relay thread** that reads bytes from the CLI UART's TX pipe and writes
   them directly to Linux stdout fd 1.
4. Replacing `sysSleep()` with a `nanosleep(1 ms)` yield so the dispatch loop does not
   spin at 100% CPU between ticks.
5. Providing a `main_linux.c` entry point that wires everything together.

Logger output (`stderr`) is redirected to a file `avros.log` in the working directory
so the terminal shows only CLI I/O; the log can be tailed in a second terminal.

### Additional Files

```
test/
  console/
    main_linux.c       Entry point: raw terminal setup, bridge threads, dispatch loop
    terminal.c         termios raw mode setup and atexit restore
    terminal.h
    sys_sleep.c        sysSleep() via nanosleep(1 ms)
```

### Design Details

#### `test/console/terminal.c`

```c
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include "terminal.h"

static struct termios saved_termios;

static void restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_termios);
}

void terminal_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &saved_termios);
    atexit(restore_terminal);
    struct termios raw = saved_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_cflag |=  CS8;
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
```

#### `test/console/sys_sleep.c`

```c
#include <time.h>
#include "../../sys/sys.h"

void sysSleep(void) {
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000L }; /* 1 ms */
    nanosleep(&ts, NULL);
}
```

This file provides the strong `sysSleep` definition; the original in `sys/sys.c` is
renamed at compile time with `-DsysSleep=sysSleep_avr` (same pattern as `sysInitTick`).

#### `test/console/main_linux.c`

```c
#include "../../avrOS.h"
#include "terminal.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>   /* real glibc stdio for freopen */

extern int uartTestGetRxFd(UART_t *uart);
extern int uartTestGetTxFd(UART_t *uart);
/* cliUart is declared by ADD_UART_RW in app/avrOS_example/main.c */
extern const UART_t cliUart;

static void *stdin_bridge(void *arg) {
    int rxfd = uartTestGetRxFd((UART_t *)arg);
    uint8_t c;
    while (read(STDIN_FILENO, &c, 1) == 1)
        write(rxfd, &c, 1);
    return NULL;
}

static void *stdout_relay(void *arg) {
    int txfd = uartTestGetTxFd((UART_t *)arg);
    uint8_t c;
    while (read(txfd, &c, 1) == 1)
        write(STDOUT_FILENO, &c, 1);
    return NULL;
}

int main(void) {
    linux_hal_init();

    /* Route logger output to a file; leave stdout for CLI I/O */
    freopen("avros.log", "w", stderr);

    terminal_raw_mode();

    sysInit();
    sei();   /* no-op on Linux */

    pthread_t bridge_tid, relay_tid;
    pthread_create(&bridge_tid, NULL, stdin_bridge, (void *)&cliUart);
    pthread_create(&relay_tid,  NULL, stdout_relay,  (void *)&cliUart);

    while (1) {
        fsmDispatch();
        sysSleep();
    }
    return 0;
}
```

#### Logger UART on Linux

The example `main.c` routes the logger to `USART1` via `ADD_UART_WRITE(logUart, ...)`.
On Linux `uartInit()` for a write-only UART creates only a TX pipe. `logInit()` then sets
`stderr` to the logger's FILE stream, so all `INFO`/`WARN`/`ERR` macros write to the TX
pipe. The TX relay thread for the logger writes to the redirected `stderr` file descriptor
(`avros.log`). No changes to `srv/log.c` or `main.c` are required.

### Makefile Target

Added to `test/makefile` as a `console` target:

```makefile
CONSOLE_SRC = console/main_linux.c console/terminal.c console/sys_sleep.c

CFLAGS_CONSOLE = -Wall -std=gnu99 -g -DLINUX_HAL -DQUE_STATS -DEVNT_STATS \
                 -DFSM_STATS -DUART_STATS -DGPIO_STATS -DDEBUG -DCLI \
                 -DsysInitTick=sysInitTick_avr -DsysSleep=sysSleep_avr \
                 -Ihal -I.. -I../app/avrOS_example \
                 -include fio/fio_linux.h

console: $(CORE) $(SRV) $(LINUX_DRV) $(CONSOLE_SRC) $(HAL) $(FIO) \
         ../app/avrOS_example/main.c
	$(CC) $(CFLAGS_CONSOLE) $^ -o avros_console -lrt -lpthread
	@echo "Run:  ./avros_console"
	@echo "Log:  tail -f avros.log"
```

### Phase 9 AI Prompt

> You are implementing Phase 9 of the avrOS Linux HAL — an interactive console
> application. Phases 1–8 provide the full HAL, Linux drivers, FIO shim, and test suite.
> Phase 9 builds the same avrOS kernel as a runnable Linux binary where a user interacts
> with the avrOS CLI in real time from a terminal. **No existing source file may be
> modified.**
>
> **Task:** Create the following four files and update `test/makefile`.
>
> **`test/console/terminal.h`**
> - Include guard `TERMINAL_H`.
> - Declare `void terminal_raw_mode(void)` — enables raw terminal input and registers an
>   `atexit` handler to restore the original terminal settings.
>
> **`test/console/terminal.c`**
> - Include `<termios.h>`, `<unistd.h>`, `<stdlib.h>`, `"terminal.h"`.
> - Save the original `termios` settings from `STDIN_FILENO` via `tcgetattr`.
> - Register `restore_terminal` via `atexit`; it calls `tcsetattr(TCSAFLUSH, saved)`.
> - `terminal_raw_mode()`: copy saved settings; clear `ECHO`, `ICANON`, `ISIG`,
>   `IEXTEN` from `c_lflag`; clear `IXON`, `ICRNL`, `BRKINT`, `INPCK`, `ISTRIP` from
>   `c_iflag`; set `CS8` in `c_cflag`; set `VMIN=1`, `VTIME=0`; call `tcsetattr`.
>
> **`test/console/sys_sleep.c`**
> - Include `<time.h>` and `"../../sys/sys.h"`.
> - Implement `void sysSleep(void)` as `nanosleep` with a 1 ms interval.
> - This provides the strong definition that overrides the AVR version in `sys/sys.c`,
>   which is renamed via `-DsysSleep=sysSleep_avr` in the console compile flags.
>
> **`test/console/main_linux.c`**
> - Include `"../../avrOS.h"` (with Linux HAL flags active via `-include` and `-D` flags).
> - Include `"terminal.h"`, `<pthread.h>`, `<unistd.h>`.
> - Include `<stdio.h>` (the real glibc one, not the avrOS shim — use a guard or include
>   it before the `-include fio_linux.h` takes effect by including it directly here).
> - Declare `extern int uartTestGetRxFd(UART_t *uart)` and
>   `extern int uartTestGetTxFd(UART_t *uart)` (implemented in `test/linux_drv/uart.c`).
> - Declare `extern const UART_t cliUart` (defined by `ADD_UART_RW` in `main.c`).
> - Implement `static void *stdin_bridge(void *arg)`: reads raw bytes from
>   `STDIN_FILENO` one byte at a time; writes each to `uartTestGetRxFd(arg)`. Exits when
>   `read` returns ≤ 0.
> - Implement `static void *stdout_relay(void *arg)`: reads bytes from
>   `uartTestGetTxFd(arg)` one at a time; writes each to `STDOUT_FILENO`. Exits when
>   `read` returns ≤ 0.
> - Implement `int main(void)`:
>   1. Call `linux_hal_init()`.
>   2. Call `freopen("avros.log", "w", stderr)` to redirect logger output to a file.
>   3. Call `terminal_raw_mode()`.
>   4. Call `sysInit()`.
>   5. Call `sei()`.
>   6. Spawn `stdin_bridge` and `stdout_relay` threads via `pthread_create` passing
>      `(void *)&cliUart` as the argument to each.
>   7. Enter `while(1) { fsmDispatch(); sysSleep(); }`.
>
> **Update `test/makefile`:**
> - Add variables `CONSOLE_SRC`, `CFLAGS_CONSOLE` as specified above.
> - Add `console` target that compiles all required source files into `avros_console`.
> - Add `console` to the list of targets cleaned by `clean`.
> - After a successful build, print:
>   `@echo "Run: ./avros_console   |   Log: tail -f avros.log"`
>
> After implementing, verify the binary builds and launches without crashing:
> `make -C test console && echo BUILD_OK`

---

## Implementation Sequence and Dependencies

```
Phase 1 ──► Phase 2 ──► Phase 3 ──► Phase 4
                │                       │
                └───────────────────────┤
                                        ▼
                                    Phase 5 (Unity — independent)
                                        │
                              ┌─────────┴──────────┐
                              ▼                    ▼
                          Phase 6             Phase 7
                         (unit tests)    (integration tests)
                              └─────────┬──────────┘
                                        ▼
                                    Phase 8
                              (makefile + coverage
                               + memcheck)
                                        │
                                        ▼
                                    Phase 9
                              (interactive console
                                 application)
```

Phase 5 (Unity download) can be done in parallel with Phases 3–4. Phase 9 depends on
Phase 8 (the makefile infrastructure) but is otherwise independent of the test phases —
it can be developed in parallel with Phases 6–7 once Phases 1–4 are complete.

---

## Known Risks and Mitigations

| Risk | Mitigation |
|---|---|
| `sysInitTick` link-order conflict between `sys.c` and `linux_drv/sys_tick.c` | Rename with `-DsysInitTick=sysInitTick_avr` on the `sys.c` compile step only |
| `sysSleep` link-order conflict between `sys.c` and `console/sys_sleep.c` | Same pattern: `-DsysSleep=sysSleep_avr` on the `sys.c` compile step in the console build only |
| `fio.h` `FILE->buf` field access incompatible with glibc `FILE` | Custom `FILE` typedef in `fio_linux.h`, injected via `-include` before all sources |
| `drv/mem.h` inline functions reference undefined AVR linker symbols | `linux_mem_syms.h` provides Linux-compatible definitions; guarded with `#ifdef LINUX_HAL` |
| `ATOMIC_BLOCK` in `sys.c` and `uart.c` used from ISR context — not thread-safe on Linux | Tests run single-threaded for unit tests; integration tests use a mutex in the HAL tick thread where race conditions are possible — acceptable for test purposes |
| `ADD_UART_RW` macro initializes `FILE` with `{.buf=..., .put=..., .get=..., .flags=..., .udata=...}` — these are struct designators for the custom `FILE` type, so they only work if the custom `FILE` typedef is visible at the call site | The `-include test/fio/fio_linux.h` flag ensures it is always visible before any source file is parsed |
| `cli.c` calls `fioWaitInput(stdin)` which uses the event system to suspend the FSM until input is available — this depends on the RX queue event being triggered by the UART reader thread | The pipe-backed UART reader thread calls `evntTrigger(queGetEvent(uart->rxQueue), ...)` after each `quePutByte`, which is the same mechanism the real ISR uses; this should work correctly |
| Console app: terminal not restored on crash or SIGKILL | `atexit` handles normal exit and `SIGTERM`/`SIGINT`; for `SIGKILL` the kernel resets the terminal when the controlling process exits — no additional handling needed |
| Console app: logger output mixed with CLI output on the same terminal | Logger UART TX pipe is relayed to `avros.log` via `freopen("avros.log", "w", stderr)`; CLI and logger are on separate UARTs and therefore separate pipes |
