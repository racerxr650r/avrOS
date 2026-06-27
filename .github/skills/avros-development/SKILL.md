---
name: avros-development
description: 'Develop, modify, or extend the avrOS cooperative RTOS for Microchip AVR-Dx microcontrollers. Use when writing or reviewing C code under drv/, srv/, sys/, or app/; adding new drivers, services, kernel modules, or state machines; touching event/queue/FSM/CLI/UART/GPIO/DAC code; working with ATOMIC_BLOCK or ISRs on AVR; adding ADD_* registration macros, SECTION linker tables, or initializer hooks; questions about AVR-Dx peripherals (TCB, USART, GPIO, DAC, sleep modes) in the context of this project.'
---

# avrOS Development

Specialized workflow for working on the avrOS codebase — a lightweight
cooperative FSM-based RTOS for the Microchip AVR-Dx (AVR128DA family).

## When to use

- Adding or modifying source under `drv/`, `srv/`, `sys/`, or `app/`.
- Creating a new module (driver, service, kernel piece) and need to
  follow the project's descriptor/status + `ADD_<THING>` pattern.
- Touching anything that interacts with an ISR, the FSM scheduler,
  events, queues, or the system tick.
- Reviewing a change for compliance with project conventions.
- Answering AVR-Dx peripheral questions in the context of this code.

Do *not* use this skill for:
- Pure host-side tooling under `util/` (scripts, `wav2c`, `snd2c`).
- Documentation-only edits (Doxygen `.md` files unrelated to code).

## Core references (load on demand)

When working in this project, consult these documents — do not guess
at conventions or peripheral behavior.

| Reference | Use for |
|-----------|---------|
| [doc/CODING_STANDARDS.md](../../../doc/CODING_STANDARDS.md) | File layout, naming, formatting, descriptor/status split, `ADD_<THING>` macro pattern, ATOMIC_BLOCK rules, CLI command shape, module checklist. |
| [doc/SDD.md](../../../doc/SDD.md) | Architecture, kernel/service/driver module responsibilities, event/queue/FSM data flow, configuration knobs, application interface. |
| [doc/MANUAL.md](../../../doc/MANUAL.md) | End-user / integrator view of how to build and configure an avrOS application. |
| [doc/Microchip/AVR128DA28-32-48-64-Data-Sheet-40002183C.pdf](../../../doc/Microchip/AVR128DA28-32-48-64-Data-Sheet-40002183C.pdf) | Authoritative peripheral register details for the target part. |
| [doc/Microchip/AVR128DA-28-32-48-64-SilConErrataClarif-DS80000882.pdf](../../../doc/Microchip/AVR128DA-28-32-48-64-SilConErrataClarif-DS80000882.pdf) | Silicon errata — check before blaming code for unexpected peripheral behavior. |
| [doc/Microchip/AN3429-Getting-Started-AVRDA-Family-DS00003429B.pdf](../../../doc/Microchip/AN3429-Getting-Started-AVRDA-Family-DS00003429B.pdf) | High-level intro to the AVR-Dx family. |
| [doc/Microchip/AVR-DA-Product-Brief-DS40002164B.pdf](../../../doc/Microchip/AVR-DA-Product-Brief-DS40002164B.pdf) | Feature summary. |
| [doc/Microchip/AVR-InstructionSet-Manual-DS40002198.pdf](../../../doc/Microchip/AVR-InstructionSet-Manual-DS40002198.pdf) | Instruction-level reference when reading/writing inline asm or analyzing generated code. |
| [doc/Microchip/Getting-Started-with-GPIO-DS90003229B.pdf](../../../doc/Microchip/Getting-Started-with-GPIO-DS90003229B.pdf) | Use when editing `drv/gpio.[ch]` or wiring new pin-change events. |
| [doc/Microchip/TB3214-Getting-Started-with-TCB-DS90003214.pdf](../../../doc/Microchip/TB3214-Getting-Started-with-TCB-DS90003214.pdf) | System tick (`sys.c`) lives on a TCB — required reading for tick changes. |
| [doc/Microchip/TB3212-Getting-Started-with-TCD-DS90003212.pdf](../../../doc/Microchip/TB3212-Getting-Started-with-TCD-DS90003212.pdf) | TCD timer reference for PWM/audio work. |
| [doc/Microchip/TB3218-Getting-Started-with-CCL-DS90003218.pdf](../../../doc/Microchip/TB3218-Getting-Started-with-CCL-DS90003218.pdf) | Configurable Custom Logic — for peripheral-routing tricks. |
| [doc/Microchip/Using-10Bit-DAC-for-Generating-Analog-Signals-DS90003235C.pdf](../../../doc/Microchip/Using-10Bit-DAC-for-Generating-Analog-Signals-DS90003235C.pdf) | Use when editing `drv/dac.[ch]` or `srv/pcm.c`. |
| [doc/Microchip/12BitADC-Conv-Accumulation-Triggering-Events-DS90003245D.pdf](../../../doc/Microchip/12BitADC-Conv-Accumulation-Triggering-Events-DS90003245D.pdf) | Reference for adding an ADC driver. |
| [doc/Microchip/AVRDA-Low-Power-Features-SleepModes-DS00003664A.pdf](../../../doc/Microchip/AVRDA-Low-Power-Features-SleepModes-DS00003664A.pdf) | Use when modifying `sysSleep()` or evaluating wake sources. |
| [doc/Microchip/AN2447-Measure-Battery-Voltage.pdf](../../../doc/Microchip/AN2447-Measure-Battery-Voltage.pdf) | Application note for battery monitoring. |

## Hard rules (do not break)

These are derived from `doc/CODING_STANDARDS.md` and are non-negotiable
in this codebase:

1. **No dynamic allocation.** No `malloc`, `calloc`, `free`, or
   variable-length arrays in OS or service code. All objects are
   registered statically via an `ADD_<THING>` macro.
2. **No blocking in mainline code.** State handlers run to completion.
   Use `fsmWaitTicks` / `fsmWaitMilliseconds` for delay, and
   `evntWait` / `queWait` for I/O.
3. **ISRs are thin and non-blocking.** No `printf`, no logger calls,
   no busy-waits. Delegate to a `static` helper if useful.
4. **Shared state is `volatile` and accessed inside
   `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)`.** Single-byte AVR reads/writes
   are *not* automatically atomic from C.
5. **Descriptors live in flash via `SECTION(<MOD>_TABLE)`**; runtime
   status lives in RAM. Don't mix the two.
6. **Public symbols use the module prefix** (`evnt`, `que`, `gpio`,
   `fsm`, `uart`, `cli`, `log`, `sys`, …). One prefix per module.
7. **Include `avrOS.h` (or `../avrOS.h`) at the top of every `.c`
   file** — it pulls libc, AVR headers, and the OS API.
8. **Doxygen on every public symbol** in headers (`@brief`, `@param`,
   `@return`).
9. **Manipulate peripheral registers only through the driver header.**
   All new source code must use the inline functions in the matching
   `drv/<periph>.h` (e.g. `tcb.h`, `tca.h`, `rtc.h`, `clk.h`, `slp.h`,
   `rst.h`, `nvm.h`, `wdt.h`, `int.h`, `evt.h`, `vref.h`, `adc.h`,
   `ac.h`, `zcd.h`, `dac.h`, `gpio.h`) instead of writing the
   `PERIPH.REG` registers directly. If no driver exists for the
   peripheral, add one following the driver pattern. Drivers themselves
   delegate to existing drivers for shared resources (e.g. ADC/AC/DAC
   reference selection calls `vref.h`).
10. **Keep `doc/groups.dox` in sync with Doxygen.** When a new header
    introduces an `@addtogroup`, add the matching
    `@defgroup … @ingroup …` entry to
    [doc/groups.dox](../../../doc/groups.dox); when a module's scope
    changes, update its existing group `@brief` / description.

## Standard procedures

### Adding a new module (driver / service / kernel piece)

1. Pick a 3–4 letter lowercase prefix and the correct directory
   (`drv/`, `srv/`, `sys/`).
2. Create `mod.h` and `mod.c` with the project file banner (see any
   existing file in the same directory).
3. Wrap the header in `#ifndef MOD_H_` and a Doxygen
   `@addtogroup mod_<layer>` / `@{` … `@}` block.
4. Define the descriptor (`<mod>Descriptor_t`, `const` in flash) and
   the status (`<mod>_t`, `volatile` in RAM) types.
5. Implement an `ADD_<THING>(name, ...)` macro that:
   - places the descriptor in `SECTION(<MOD>_TABLE)`,
   - declares the runtime status,
   - calls `ADD_INITIALIZER(name, modInit, (void *)&name)`,
   - uses `CONCAT(name, _suffix)` for derived symbols,
   - uses `DEFAULT_OR_ARG(...)` for optional trailing args.
6. Implement `int modInit(const fsmStateMachineDescr_t *)` that walks
   `__start_MOD_TABLE` / `__stop_MOD_TABLE` and initializes each
   descriptor.
7. Add `#ifdef MOD_CLI` / `ADD_COMMAND("mod", modCmd, true)` for
   runtime introspection.
8. Add `#ifdef MOD_STATS` fields to the status struct.
9. Add the new header to `avrOS.h` in the correct section.
10. Add a matching `@defgroup … @ingroup …` entry to
    [doc/groups.dox](../../../doc/groups.dox) for the header's
    `@addtogroup`.
11. Walk the checklist at the end of
    [doc/CODING_STANDARDS.md](../../../doc/CODING_STANDARDS.md).

### Touching ISR-shared state

1. Mark the variable `volatile`. Pointers shared with ISRs are
   `T * volatile name`.
2. Read or modify it only inside `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)`
   in mainline code.
3. ISRs implicitly run with interrupts disabled — no nested atomic
   block needed.
4. Communicate from ISR to mainline through queues
   (`quePut`), events (`evntTrigger`), or shared flags — never a
   direct callback into FSM-managed code.

### Modifying a peripheral driver

1. Open the relevant Microchip reference for the peripheral (see
   table above).
2. Check
   [doc/Microchip/AVR128DA-28-32-48-64-SilConErrataClarif-DS80000882.pdf](../../../doc/Microchip/AVR128DA-28-32-48-64-SilConErrataClarif-DS80000882.pdf)
   before assuming buggy code.
3. Preserve the ISR → `isrFooBar()` helper pattern used in `drv/uart.c`.
4. Keep all register writes in the init path inside an
   `ATOMIC_BLOCK` if the peripheral can fire interrupts during setup.
5. If the driver introduces a new event, document the sub-type
   constants in the driver header.

### CLI commands

Every introspectable module exposes a `<mod>Cmd(int argc, char *argv[])`
function compiled in under `#ifdef <MOD>_CLI`. Conventions:

- Return `0` on success, `-1` on usage / lookup failure.
- Support both "list all" (no extra argv) and "show one by name"
  (`argv[1]`) by iterating the section table.
- Use the colour macros from `srv/cli.h` (`BOLD`, `FG_BLUE`,
  `UNDERLINE`, `RESET`). Always `RESET` before the newline.

## Pre-commit checklist

Run through this list before declaring a change complete:

- [ ] File headers (banner + Doxygen) present and unchanged in style.
- [ ] Module prefix consistent on every new public symbol.
- [ ] `ADD_<THING>` macro (if added) registers via `SECTION(...)` *and*
      `ADD_INITIALIZER(...)`.
- [ ] ISR-shared state is `volatile` and only mutated inside
      `ATOMIC_BLOCK`.
- [ ] No new `malloc` / `free` / VLA / blocking call paths.
- [ ] `<MOD>_STATS` and `<MOD>_CLI` gates added (or intentionally
      omitted with comment).
- [ ] Doxygen comments on every public symbol in the header.
- [ ] Peripheral register access goes through the `drv/<periph>.h`
      driver — no direct `PERIPH.REG` writes in new code.
- [ ] New/changed Doxygen `@addtogroup` has a matching `@defgroup`
      entry in `doc/groups.dox`.
- [ ] New module included from `avrOS.h` in the correct section.
- [ ] Builds clean for the example app (`app/avrOS_example`).
- [ ] CLI command produces sensible output for both forms.

## Deep-dive references (load on demand)

Every per-topic document now lives with the rest of the project docs.
Load only the one(s) relevant to the current task.

| Topic | Canonical home |
|-------|----------------|
| Build / flash workflow, make targets, programmer config, common build failures | [doc/MANUAL.md — Make Targets Reference](../../../doc/MANUAL.md#make-targets-reference) + [app/avrOS_example/makefile](../../../app/avrOS_example/makefile) |
| `avrOSConfig.h` macro reference (CPU, tick, logger, CLI, `<MOD>_CLI` / `<MOD>_STATS`) | [doc/MANUAL.md — avrOSConfig.h Reference](../../../doc/MANUAL.md#avrosconfigh-reference) + [app/avrOS_example/avrOSConfig.h](../../../app/avrOS_example/avrOSConfig.h) |
| Runtime CLI commands, failure-signature triage | [doc/MANUAL.md — Runtime Debugging](../../../doc/MANUAL.md#runtime-debugging) |
| Memory layout / flash window / `mem.h` helpers | [doc/SDD.md §3.4](../../../doc/SDD.md#34-memory-layout) |
| Linker sections / descriptor tables / adding `<MOD>_TABLE` | [doc/SDD.md §3.5](../../../doc/SDD.md#35-linker-sections-and-descriptor-tables) + [app/avrOS_example/avrOS.x](../../../app/avrOS_example/avrOS.x) |
| State-machine priority encoding (class + sub-priority) | [doc/SDD.md §4.2.3](../../../doc/SDD.md#423-priority-levels) + Doxygen on `fsmType_t` in [sys/fsm.h](../../../sys/fsm.h) |
| Event sub-type contract (`evntType` / `trigger`) | [doc/SDD.md §4.3.4](../../../doc/SDD.md#434-event-sub-type-contract) + Doxygen on `event_t` in [sys/event.h](../../../sys/event.h) |
| Verification, testing, regression checklist | [doc/SDD.md §9](../../../doc/SDD.md#9-verification-and-testing) |
| cppcheck / complexity invocation and triage | [doc/CODING_STANDARDS.md §12](../../../doc/CODING_STANDARDS.md#12-static-analysis-and-complexity) |

Promoted to canonical project files (load directly when relevant):

| File | Use for |
|------|---------|
| [.editorconfig](../../../.editorconfig) | Editor-agnostic formatting rules (tabs, EOL, trailing whitespace). |
| [.clang-format](../../../.clang-format) | Optional mechanical reformat matching `doc/CODING_STANDARDS.md`. |
| [util/scripts/stamp_license.sh](../../../util/scripts/stamp_license.sh) | Generate the BSD-style header for new `.c` / `.h` files. |
| [CONTRIBUTING.md](../../../CONTRIBUTING.md) | Branch / commit / PR / tag conventions. |
| [.github/copilot-instructions.md](../copilot-instructions.md) | Always-on routing instructions that point at this skill. |
