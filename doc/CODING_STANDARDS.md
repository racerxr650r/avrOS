# avrOS Coding Standards

This document captures the conventions actually used in the avrOS source
tree (`drv/`, `srv/`, `sys/`). It is descriptive — derived from the existing
code — so that new modules can be added in a style consistent with what is
already there.

The intent is *not* to introduce new rules. Where the existing code is
inconsistent, both forms are noted and the more common one is recommended.

---

## 1. Project layout

| Directory | Purpose | Priority constant |
|-----------|---------|-------------------|
| `drv/`    | Hardware drivers (CPU, GPIO, UART, DAC, MEM) | `FSM_DRV` |
| `sys/`    | OS kernel pieces (FSM scheduler, events, queues, system tick, fio, list) | `FSM_SYS` |
| `srv/`    | Higher-level services (CLI, logger, button, PCM) | `FSM_SRV` |
| `app/`    | Application(s) built on top of the OS | `FSM_APP` |
| `util/`   | Host-side tooling (scripts, `wav2c`, `snd2c`) | n/a |

All OS-facing headers are pulled in via the umbrella header
`avrOS.h` (which itself includes `avrOSConfig.h`). Source files in
`drv/`, `srv/`, and `sys/` therefore start with a single include:

```c
#include "avrOS.h"          // or "../avrOS.h" from sys/
```

Application source files (under `app/`) do the same.

---

## 2. File organization

### 2.1 File header

Every `.c` and `.h` file begins with a banner comment block containing:

- File name (`.c` files use a plain block; `.h` files use a Doxygen
  `@file` / `@brief` block).
- One-line description / longer description.
- `Created:` date and `Author:` line.
- Copyright notice and the BSD-style "permission to use" disclaimer used
  throughout the project.

Example (`.h`):

```c
/**
 * @file uart.h
 * @brief UART driver — interrupt-driven USART with queue buffering ...
 *
 * Created: 2/28/2021 4:14:29 PM
 * Author: john anderson
 *
 * Copyright (C) 2021 by John Anderson <racerxr650r@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software ...
 */
```

### 2.2 Header guard

Use traditional `#ifndef`/`#define`/`#endif` guards. The two patterns
present in the tree are both acceptable:

```c
#ifndef UART_H_
#define UART_H_
...
#endif  // UART_H_
```

```c
#ifndef __FSM_H
#define __FSM_H
...
#endif
```

New files should prefer the trailing-underscore form (`MODULE_H_`) — it
is the dominant style and avoids the reserved `__` prefix.

### 2.3 Doxygen group tag

Each public header opens a Doxygen group right after the guard:

```c
/** @addtogroup uart_driver
 * @{
 */
...
/** @} */ // end of uart_driver
```

Group names follow `<module>_<layer>` (e.g. `gpio_driver`, `fsm_manager`,
`cli_service`, `sys_kernel`, `event_manager`, `log_service`).

### 2.4 Section banners

Inside each file, code is grouped by purpose using a fixed set of banner
comments. The set used in practice is:

```c
// Includes -------------------------------------------------------------------
// Constants ------------------------------------------------------------------
// Data Types -----------------------------------------------------------------
// Macros ---------------------------------------------------------------------
// Externs --------------------------------------------------------------------
// Internal Globals -----------------------------------------------------------
// Internal Function Prototypes -----------------------------------------------
// Interrupt Table Hooks ------------------------------------------------------
// Interrupt Handler Functions ------------------------------------------------
// Command line interface -----------------------------------------------------
// CLI Commands ---------------------------------------------------------------
// Inline Functions -----------------------------------------------------------
// External Functions ---------------------------------------------------------
// Internal Functions ---------------------------------------------------------
```

Banner text is followed by a long run of `-` (or `*`) characters so the
banner reaches roughly column 80. Pick the closest matching banner from
this list rather than inventing new ones.

### 2.5 Include order in source files

```c
#include "avrOS.h"           // umbrella header
#include <stdlib.h>          // additional libc headers, only when needed
#include <stdio.h>
#include <string.h>
```

`avrOS.h` already pulls in `stdio.h`, `stdlib.h`, `string.h`, `stdbool.h`,
and the AVR headers, so most modules need only the umbrella include.

---

## 3. Naming conventions

### 3.1 Identifiers

| Kind | Style | Example |
|------|-------|---------|
| Function | `lowerCamelCase` with a 2–4 letter module prefix | `evntArm`, `queGet`, `uartGetChar`, `gpioSetOutput`, `fsmDispatch` |
| Static (file-scope) function | Same as public, with no extra prefix | `static void isrUsartDRE(...)` |
| Module-scope (file-static) variable | `gs` prefix for "global static" is used in some drivers; otherwise plain camelCase | `static UART_t *gsUart0`, `static uint32_t scanCycle` |
| Typedef (struct / enum) | `lowerCamelCase` ending in `_t` | `event_t`, `gpio_t`, `queue_t`, `fsmStateMachine_t` |
| Underlying tag of typedef'd struct | `UPPER_SNAKE_CASE` | `struct GPIO_TYPE`, `struct STATE_MACHINE_TYPE` |
| Enum constant | `UPPER_SNAKE_CASE` | `GPIO_INPUT`, `EVENT_ARMED`, `QUE_EVENT_FULL` |
| `#define` constant | `UPPER_SNAKE_CASE` | `SYS_TICK_TIMER`, `MAX_CMD_LINE` |
| Function-like `#define` that registers a static object | `ADD_<THING>` | `ADD_GPIO`, `ADD_EVENT`, `ADD_QUEUE`, `ADD_STATE_MACHINE`, `ADD_COMMAND`, `ADD_LOG`, `ADD_UART_RW`, `ADD_INITIALIZER` |
| Function-pointer typedef | `lowerCamelCase` ending in `_t` (typically `Handler_t`) | `evntHandler_t`, `fsmHandler_t`, `gpioHandler_t`, `commandHandler_t` |

Module prefixes currently in use:

- `cpu`, `dac`, `gpio`, `mem`, `uart` — drivers
- `fsm`, `evnt`, `que`, `fio`, `sys` — kernel
- `cli`, `log`, `btn` — services

When adding a new module, choose a short (3–4 letter) lowercase prefix
and apply it consistently to every public function and `ADD_*` macro.

### 3.2 Section / linker symbols

OS tables placed in custom linker sections use `UPPER_SNAKE_CASE` ending
in `_TABLE`:

```
FSM_TABLE, EVNT_TABLE, QUE_TABLE, GPIO_TABLE, UART_TABLE, CLI_CMDS
```

The descriptor pointer iteration pattern (see §6.3) requires the
`__start_<NAME>` / `__stop_<NAME>` symbols generated by the linker for
each named section.

### 3.3 Configuration macros

Per-module feature flags follow the patterns:

- `<MOD>_CLI`   — register CLI sub-commands for the module
- `<MOD>_STATS` — compile in runtime statistics

Examples found in the code: `UART_STATS`, `QUE_STATS`, `EVNT_STATS`,
`GPIO_STATS`, `FSM_STATS`, `LOG_LEVEL`, `LOG_FORMAT`.

All of these are expected to be defined (or not) in `avrOSConfig.h`.

---

## 4. Formatting

- **Indentation:** tabs, displayed as 4 columns. Body of every block is
  one tab deeper than its brace.
- **Braces:** Allman / BSD style — opening brace on its own line for
  every function, `if`, `else`, `for`, `while`, `do`, `switch`.
- **Blank line** between functions; banner comments are immediately
  followed by code (no blank line).
- **One statement per line.** Multiple variable declarations on one
  line are tolerated for related items (`int a, b, c;`) but the
  prevailing style is one declaration per line.
- **Spacing around operators:** standard — `a + b`, `x = y`, but
  control-flow keywords are written *without* a space before the
  parenthesis: `if(...)`, `while(...)`, `for(...)`, `switch(...)`.
- **`return`** is also written with a parenthesized value and no space:
  `return(ret);` — this is a project-wide idiom; follow it.
- **Pointer asterisk** is bound to the type for variable declarations
  in headers (`UART_t *uart;`) and bound to the name in some `.c`
  files. Either is acceptable; match the surrounding file.
- **Line length:** target ~100 columns. Long descriptor initializer
  lines inside `ADD_*` macros are allowed to exceed this.

### 4.1 Comments

- Use `//` for short single-line internal comments and `/* ... */` or
  `/** ... */` for Doxygen blocks.
- Public types, functions, and `ADD_*` macros in headers get Doxygen
  comments with `@brief`, parameter docs (`@param`), and `@return`.
- Internal (`static`) functions in `.c` files get a one-line `//`
  comment immediately above the definition describing intent.
- Avoid commented-out code in committed files. Where it exists today
  it is bracketed with a clear marker (`/*#else ... #endif*/`) and
  preserved intentionally; do not add more.

---

## 5. Type and language conventions

- **C standard:** C99 / GNU C as supplied by `avr-gcc`. Designated
  initializers (`.field = value`) are used everywhere for static
  descriptor tables and are required for new code.
- **Fixed-width integers:** prefer `uint8_t`, `uint16_t`, `uint32_t`,
  `int16_t`, etc. from `<stdint.h>` (pulled in transitively).
- **Booleans:** use `bool` / `true` / `false` from `<stdbool.h>`.
- **`const`-correctness:**
  - Descriptor tables placed in flash are always `const static <type> SECTION(...) name = { ... };`.
  - Runtime mutable state objects (queue head/tail, event state,
    state-machine status) are *not* `const`, and are typically
    marked `volatile` because they are touched by ISRs.
- **`volatile`:** apply to any variable accessed from both an ISR and
  mainline code. Pointers shared with ISRs use `T * volatile name`
  (see `drv/uart.c` `gsUart0`).
- **Inline functions:** small accessors live in headers as
  `static inline` (see `evntGetStatus`, `queIsEmpty`, `memTextSize`,
  `percentWhole`). Use them in preference to function-like macros
  when type checking is desired.
- **`UNUSED(x)`** macro from `avrOS.h` is used to silence "unused
  parameter" warnings in CLI command handlers.

### 5.1 Atomicity

All read-modify-write sequences on data shared with ISRs are wrapped in
`ATOMIC_BLOCK(ATOMIC_RESTORESTATE)` from `<util/atomic.h>`. The block is
opened immediately followed by the comment `// Start of critical
section` and closed with `// End of critical section` (see `queue.c`,
`event.c`).

### 5.2 ISR style

```c
ISR(USART0_RXC_vect)
{
    if(gsUart0 != NULL)
        isrUsartRXC(gsUart0);
}
```

Interrupt vectors are *thin* — they delegate to a `static` helper
(`isrFooBar`) so the same handler can serve multiple vectors. Keep ISR
bodies short and avoid `printf`/blocking calls.

---

## 6. Module structure pattern

Every OS module under `drv/`, `srv/`, or `sys/` follows the same
register / dispatch pattern. Adopt this pattern for new modules.

### 6.1 Descriptor + status split

For each kind of object the module manages, define two types:

- A **descriptor** (`<thing>Descriptor_t` or just `<thing>_t` when the
  object is entirely const) — `const` data placed in flash via
  `SECTION(<MOD>_TABLE)`.
- A **status** (`<thing>_t`) — RAM-resident runtime state, often
  `volatile`, with a `descr` back-pointer when statistics are enabled.

```c
typedef struct EVENT_TYPE {
    evntState_t                       state;
    volatile fsmStateMachine_t       *stateMachine;
    ...
#ifdef EVNT_STATS
    const struct EVENT_DESCR_TYPE    *descr;
    evntStats_t                       stats;
#endif
} event_t;

typedef struct EVENT_DESCR_TYPE {
    char           *name;
    event_t        *status;
    evntHandler_t   handler;
} evntDescriptor_t;
```

Stats fields are wrapped in `#ifdef <MOD>_STATS` so they can be
compiled out for size-constrained builds. The `name` field generally
lives in the descriptor and is also `#ifdef <MOD>_STATS` in some
modules (gpio, uart) and unconditional in others (events, queues);
prefer making it conditional in new code.

### 6.2 Registration macro

Every module exposes an `ADD_<THING>(name, ...)` macro that:

1. Stringifies the symbol into a `.name` field.
2. Declares the descriptor `const static ... SECTION(<MOD>_TABLE)`.
3. Declares the runtime status object.
4. Registers an `ADD_INITIALIZER(...)` call so the FSM scheduler will
   initialize it at startup.
5. Uses `CONCAT(name, _suffix)` to derive related symbol names
   (`<name>_descr`, `<name>_buffer`, `<name>_event`, etc.).

Optional trailing arguments use the `DEFAULT_OR_ARG(,##__VA_ARGS__,__VA_ARGS__,DEFAULT)` idiom from `avrOS.h`.

```c
#define ADD_QUEUE(queName, queSzElement, queSz) \
    static uint8_t               CONCAT(queName,_buffer)[queSz*queSzElement]; \
    const static queDescriptor_t CONCAT(queName,_descr); \
    static volatile queue_t      queName = { .head = queSz, .tail = 0, .max = 0, .descr = &CONCAT(queName,_descr) }; \
    ADD_EVENT(queName ## _evnt); \
    const static queDescriptor_t SECTION(QUE_TABLE) CONCAT(queName,_descr) = { ... };
```

### 6.3 Table iteration

Modules walk their own descriptor table using the linker-generated
section symbols:

```c
extern void *__start_QUE_TABLE, *__stop_QUE_TABLE;

queDescriptor_t *descr = (queDescriptor_t *)&__start_QUE_TABLE;
for(; descr < (queDescriptor_t *)&__stop_QUE_TABLE; ++descr)
{
    ...
}
```

This is the canonical "find by name" pattern (`evntGetEvent`,
`fsmGetStateMachine`, `gpioCmd`). Use it instead of building runtime
registries.

### 6.4 Initialization

Modules do not call each other's init functions directly. They are
registered via `ADD_INITIALIZER(name, initFn, instance)`, and
`sysInit()` walks the FSM table once on boot to invoke every
initializer. New modules expose an `xxxInit(const fsmStateMachineDescr_t *)`
function and rely on this mechanism.

### 6.5 CLI integration

Every module that exposes runtime state defines a CLI command,
conditionally compiled:

```c
#ifdef QUE_CLI
ADD_COMMAND("que", queCmd, true);
static int queCmd(int argc, char *argv[])
{
    queDescriptor_t *descr = (queDescriptor_t *)&__start_QUE_TABLE;
    for(; descr < (queDescriptor_t *)&__stop_QUE_TABLE; ++descr)
    {
        if(argc < 2 || (argc == 2 && !strcmp(descr->name, argv[1])))
            printf(...);
    }
    return(0);
}
#endif
```

Conventions for CLI handlers:

- Always `static int <name>Cmd(int argc, char *argv[])`.
- Return `0` on success, `-1` on usage / lookup failure.
- Use the colour/style macros from `cli.h`
  (`BOLD`, `UNDERLINE`, `FG_BLUE`, `RESET`, etc.).
- Reset attributes (`RESET`) before the newline.
- Filter by `argv[1]` to support both "list all" and "show one".

---

## 7. Concurrency model

- The application's `main()` calls `sysInit()` once, then loops calling
  `fsmDispatch()` followed by `sysSleep()`.
- All mainline code runs cooperatively inside FSM state handlers; one
  state handler runs to completion before another is scheduled. Do not
  block, spin, or busy-wait in a state handler — use:
  - `fsmWaitTicks(sm, n)` / `fsmWaitMilliseconds(sm, ms)` for delays,
  - `evntWait(sm, event, type)` / `queWait(que, type)` for I/O.
- ISRs are the only true-async context. They communicate with mainline
  code through:
  - queues (`quePut`, `queGet`),
  - events (`evntTrigger`),
  - shared variables protected by `ATOMIC_BLOCK`.
- ISRs must never call `printf`, the logger, or any blocking helper.

### 7.1 State-machine priority

`ADD_STATE_MACHINE` takes a `fsmPriority_t` that is the bitwise OR of a
class (`FSM_DRV`, `FSM_SYS`, `FSM_SRV`, `FSM_APP`) and a 6-bit
sub-priority. Drivers use the lowest priority class, applications the
highest. Use the class that matches the directory the module lives in.

---

## 8. Memory and program storage

- Strings stored in flash use the `ROM_STR(name, str)` / `ROM_STR_G`
  helpers from `avrOS.h` so they land in `PROGMEM`.
- Descriptor tables go in custom sections via `SECTION(NAME)`; do not
  put mutable state in those sections.
- Large RAM allocations (queue buffers, etc.) are declared `static` at
  file scope by the relevant `ADD_*` macro — no dynamic allocation
  (`malloc`/`free`) anywhere in the OS or services.
- Stack usage is monitored via `memStackFill()` / the `mem` driver;
  keep state handlers shallow and avoid large stack arrays.

---

## 9. Error handling and logging

- Functions return `int` with `0` for success and `-1` (or a negative
  error class) for failure. Some kernel calls return a typed enum
  (`evntState_t`, `bool`).
- Use the logger macros from `srv/log.h` for diagnostic output:
  `INFO`, `WARN`, `ERROR`, `CRITICAL`. The macros are compiled away
  when `LOG_LEVEL` or `LOG_FORMAT` is `0`.
- `CRITICAL` is fatal — it prints a banner and halts (`while(1);`).
  Reserve it for unrecoverable conditions.
- Never call `assert()` — the project does not link against an assert
  implementation; use `CRITICAL` if a precondition is genuinely fatal.

---

## 10. Doxygen documentation conventions

Public headers are documented with Doxygen. Required tags:

- `@file` and `@brief` at the top of each header (the `.c` files use a
  plain block).
- `@addtogroup` / `@{` … `@}` wrapping the entire header body.
- For every public function, macro, and type:
  - `@brief` short summary (one line).
  - Optional longer description paragraph.
  - `@param name` for each parameter (in order).
  - `@return` for non-void functions.
- Code references in prose use backticks (` `INFO` `, ` `EVENT_TYPE_TICK` `).
- Enum and struct members use trailing `///< description` comments.

Example:

```c
/**
 * @brief Set output pin(s) selected by a bit mask.
 *
 * Sets output pins corresponding to `value` using the OUTSET register.
 *
 * @param gpio  Pointer to the GPIO descriptor.
 * @param value Bit mask of pin(s) to set.
 */
void gpioSetOutput(const gpio_t *gpio, uint8_t value);
```

---

## 11. Checklist for a new module

When adding a new module under `drv/`, `srv/`, or `sys/`:

- [ ] `mod.h` and `mod.c` created with the project file banner.
- [ ] Header wrapped in guard and Doxygen group.
- [ ] Module prefix (3–4 letters) chosen and used on every public symbol.
- [ ] Public types use the descriptor / status split where state exists.
- [ ] `ADD_<THING>` macro registers descriptor in `SECTION(<MOD>_TABLE)`
      and calls `ADD_INITIALIZER` for the init function.
- [ ] `<MOD>_STATS` and `<MOD>_CLI` feature flags supported (or
      omitted with a comment).
- [ ] All ISR-shared state declared `volatile` and accessed only inside
      `ATOMIC_BLOCK`.
- [ ] No blocking calls outside `fsmWait*` / `evntWait` / `queWait`.
- [ ] CLI command registered when `<MOD>_CLI` is defined, following the
      `<mod>Cmd(int argc, char *argv[])` pattern.
- [ ] Module included from `avrOS.h` in the appropriate section.
- [ ] Doxygen comments on every public symbol.

---

## 12. Static analysis and complexity

`make all` runs both `cppcheck` and GNU `complexity` over the full
source set. Reports land under `app/<name>/build/`.

### 12.1 cppcheck

Invocation (see [app/avrOS_example/makefile](../app/avrOS_example/makefile)):

```
cppcheck -v --std=c99 --platform=avr8 --library=avr.cfg \
         --max-ctu-depth=10 --cppcheck-build-dir=$(BUILD_DIR)cppcheck \
         --language=c --inconclusive \
         -I$(DFP)/include -I. $(INCLUDE) $(SRCS)
```

Key flags and why they are set:

| Flag | Why |
|------|-----|
| `--std=c99` | Match the compiler (`-std=gnu99`). |
| `--platform=avr8` | 8-bit, 16-bit `int`. |
| `--library=avr.cfg` | Knowledge of AVR / avr-libc symbols. |
| `--max-ctu-depth=10` | Cross-translation-unit analysis through the descriptor tables. |
| `--cppcheck-build-dir=…` | Required for incremental analysis. |
| `--language=c` | Avoid C++ misinterpretation of `.x` linker script if it ever sneaks in. |
| `--inconclusive` | Helpful in this codebase; review each finding. |

Make targets:

| Goal | Command |
|------|---------|
| Interactive report on console | `make analyze` |
| Just the report file | `make build/cppcheck_report.txt` |
| Force a full re-analysis | `rm -r build/cppcheck && make analyze` |

Suppress false positives **inline** with a comment:

```c
// cppcheck-suppress unusedStructMember
```

Do not disable a whole class of warning project-wide without a comment
in the makefile explaining why.

Common findings in this codebase and the right response:

| Finding | Usually means |
|---------|--------------|
| `unusedStructMember` on stats fields | Member is only consumed by a CLI command compiled out by the current config — safe under `#ifdef <MOD>_STATS`. |
| `nullPointer` in `evntDispatch` / `fsmDispatch` | Loop walks a section that may be empty — guard with `if (__start_X != __stop_X)`. |
| `unusedFunction` on `*Cmd` handlers | Referenced only via the `ADD_COMMAND` section entry; suppress inline. |
| `va_list` warnings in `log.c` | Inherent to the `printf` family; suppress narrowly. |

### 12.2 Cyclomatic complexity

```
complexity -H -h -c --threshold=0 $(SRCS) > build/complexity_report.txt
```

`make complexity` prints the report. Target keep-numbers:

| Score | Meaning | Action |
|-------|---------|--------|
| 0 – 9 | Simple | Ship it. |
| 10 – 19 | Moderate | Look for early-return refactor. |
| 20 – 50 | High | Split the function. |
| > 50 | Very high | Almost always a switch-heavy state handler — break states out. |

Run `make complexity` before merging large state-machine work.
