That completely changes the architecture. Instead of an on-chip FSM dispatching non-overlapping tests in a single flash image, this is a **host-driven sequential test harness**. Every test application runs at the target's natural origin (`0x0000`), and your custom loader acts as the orchestrator.

A standard linker will refuse to link multiple `main()` functions or assign multiple sections to the same Virtual Memory Address (VMA `0x0000`) in a single pass without symbol collisions. To achieve an ELF where memory maps and symbols overlap, you have to build a **Multi-Section Container ELF** or use **GNU Linker OVERLAYs**.

Here is the implementation breakdown for building the ELF container and designing the custom loader's execution loop.

---

## The Build Pipeline: Packaging Overlapping Applications

The cleanest method to preserve individual `main()` entry points, identical vector tables at `0x0000`, and overlapping symbol names without linker errors is a **two-stage container build**: compile each test application independently, then merge their binary payloads into distinct sections of a master ELF where every section shares `VMA = 0x0000`.

1. **Compile Individual Test Applications as Standalone ELFs:** Each test gets a standard AVR memory map.
Compile and link each unit test independently. Every test gets its own `main()`, its own AVR vector table at `0x0000`, and its own `.data`/`.bss` initialization.

```bash
avr-gcc -mmcu=avr128da28 test_timer.c -o test_timer.elf
avr-gcc -mmcu=avr128da28 test_fsm.c -o test_fsm.elf

```


2. **Extract Raw Flash Payloads:** Strips ELF headers to isolate executable machine code.
Extract the pure flash image (`.text` + `.data` LMA payload) from each standalone ELF.

```bash
avr-objcopy -O binary -R .eeprom test_timer.elf test_timer.bin
avr-objcopy -O binary -R .eeprom test_fsm.elf test_fsm.bin

```


3. **Assemble the Container ELF via add-section:** Sets VMA=0x0000 for every application section.
Use `avr-objcopy` to create an empty (or stub) ELF file and inject each binary as a custom named section. Crucially, set the **section address (`VMA`) of every test section to `0x0000**` and flag them as loadable code.

```bash
# Create an empty container ELF from a dummy object or first test
avr-objcopy -I binary -O elf32-avr -B avr /dev/null test_suite_container.elf

# Inject Test 1 at VMA 0x0000
avr-objcopy --add-section .app.test_timer=test_timer.bin \
            --set-section-flags .app.test_timer=alloc,load,readonly,code \
            --change-section-address .app.test_timer=0x0000 \
            test_suite_container.elf

# Inject Test 2 at VMA 0x0000 (overlapping!)
avr-objcopy --add-section .app.test_fsm=test_fsm.bin \
            --set-section-flags .app.test_fsm=alloc,load,readonly,code \
            --change-section-address .app.test_fsm=0x0000 \
            test_suite_container.elf

```


---

## How the Custom Loader Operates

Your custom loader (running on the host machine or embedded programmer) will parse the standard 32-bit ELF Section Headers (`Elf32_Shdr`). Because every `.app.*` section has `sh_addr = 0x0000` but a distinct file offset (`sh_offset`), the loader can iterate through the suite cleanly.

```
+-------------------------------------------------------+
|                 test_suite_container.elf              |
+-------------------------------------------------------+
| ELF Header                                            |
+-------------------------------------------------------+
| Section: .app.test_timer  [sh_addr: 0x0000, size: X]  | --+-- Loads to Flash 0x0000
+-------------------------------------------------------+   |   (Test 1 execution)
| Section: .app.test_fsm    [sh_addr: 0x0000, size: Y]  | --+-- Loads to Flash 0x0000
+-------------------------------------------------------+       (Test 2 execution)
| Section Header Table (sh_offset mappings)             |
+-------------------------------------------------------+

```

### 1. The Loader Execution Loop

1. **Discover:** Open `test_suite_container.elf` and read the Section Header String Table (`shstrtab`). Filter for all sections beginning with `.app.`.
2. **Flash & Reset:**
* Read `sh_size` bytes starting from `sh_offset` in the ELF file.
* Write that payload to target Flash starting at address `sh_addr` (`0x0000`) via UPDI / bootloader interface.
* Issue a hardware reset to start execution at vector `0x0000`.


3. **Monitor Execution:** Keep the target running while polling for the "application exited" condition.
4. **Evaluate & Cycle:** Record test pass/fail results, then loop to the next `.app.*` section header.

---

## Reliable "Exit" Detection on AVR

Because bare-metal AVR applications don't return to an OS when `main()` exits, you need a deterministic hardware or memory contract between the running test and your loader:

| Exit Mechanism | How the Target Signals It | How the Loader Detects It | Best Use Case |
| --- | --- | --- | --- |
| **UPDI Hardware Breakpoint** | Execute the AVR `BREAK` instruction at test end. | Loader monitors UPDI status register (`UPDI_ASI_STAT`) for a CPU Halt state. | Hardware UPDI debuggers / bare-metal test rigs. |
| **SRAM Magic Token** | Write a completion flag + result code to a fixed SRAM address (e.g., `0x3F00`). | Loader polls SRAM address via UPDI/debug interface until the magic byte appears. | Non-intrusive polling without halting the CPU immediately. |
| **Software Reset / WDT** | Write result to `GPIOR0` register, then trigger Watchdog Reset. | Loader detects target reset and reads `GPIOR0` across the reboot. | Loaders interfacing over basic bootloaders without live UPDI control. |
| **UART Token** | Transmit an ASCII framing token (e.g., `[TEST_EXIT:PASS]`). | Loader parses target serial stream for end-of-test framing. | Continuous integration rigs with serial logging. |

---

### Alternative: GNU Linker `OVERLAY` (Shared Kernel / Single Symbol Table)

If you prefer a **single ELF with one unified DWARF symbol table** (so your debugger can inspect symbols across all apps without switching ELF files), you can use the GNU linker's `OVERLAY` command in your custom `.ld` script:

```ld
SECTIONS
{
  OVERLAY 0x0000 : AT (0x0000)
  {
    .app.test_timer { test_timer.o(.text .data) }
    .app.test_fsm   { test_fsm.o(.text .data) }
  } > FLASH
}

```

* **The Catch with `OVERLAY`:** Because all objects are linked in one invocation, **symbol names cannot collide**. Each test's entry point would need unique naming (e.g., `main_timer()`, `main_fsm()`), or must be declared `static` within its respective translation unit. If true independent `main()` symbols per app are required, the **Multi-Section Container ELF** method above is the industrial standard.
