# avrOS workspace instructions

This is the avrOS source tree — a cooperative FSM-based RTOS for
Microchip AVR-Dx microcontrollers (reference target AVR128DA28).

When working in this repository:

- Treat [doc/CODING_STANDARDS.md](../doc/CODING_STANDARDS.md) and
  [doc/SDD.md](../doc/SDD.md) as authoritative.
- For multi-step or module-level work under `drv/`, `srv/`, `sys/`, or
  `app/`, load the `avros-development` skill at
  [.github/skills/avros-development/SKILL.md](skills/avros-development/SKILL.md).
- Use the Microchip PDFs under [doc/Microchip/](../doc/Microchip/) as
  the authoritative peripheral reference — do not infer AVR-Dx
  behavior from other parts.
- Contribution conventions live in [CONTRIBUTING.md](../CONTRIBUTING.md).

## Hard constraints

- No dynamic allocation (`malloc` / `free` / VLAs).
- State handlers run to completion — no blocking outside
  `fsmWaitTicks`, `fsmWaitMilliseconds`, `evntWait`, `queWait`.
- ISRs are thin: no `printf`, no logger calls, no busy-waits.
- ISR-shared state is `volatile` and only mutated inside
  `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)`.
- Public symbols use a 3–4 letter module prefix.
- Descriptors live in flash via `SECTION(<MOD>_TABLE)`; runtime status
  in RAM.
- Allman braces, tabs (width 4), no space before `(` in `if`/`while`/
  `return`. Enforced by `.editorconfig` and `.clang-format` at the
  repo root.
