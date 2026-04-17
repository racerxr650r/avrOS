# avrOS: The Zero-Overhead, Event-Driven Runtime for AVR

[![avrOS](./images/avrOS.gif "avrOS")](https://github.com/racerxr650r/avrOS)

---
**avrOS** is a minimalist, single-stack, event-driven operating system designed to extract maximum performance and **maximum determinism** from 8-bit AVR microcontrollers. It is not a traditional RTOS with a complex software scheduler and memory-hungry task stacks; it is a minimalist framework that empowers the developer to be the master of system timing and power efficiency. 

By delegating task switching and power management to the AVR's own hardware, **avrOS** redefines what it means to be a "lean" kernel—ensuring that your application's execution is as predictable as the silicon itself.

![avrOS vs Traditional RTOS](./images/avrOS_vs_RTOS.png)

---

## Core Principles and Architecture

### 1. Hardware-Centric Task "Switching"
**avrOS** eliminates the biggest RAM and CPU cost in an RTOS: **software context switching**.
* **One Stack:** The entire system—including all state machines and ISRs—runs on a single shared stack. This makes the most efficient use of the limited SRAM found on AVR microcontrollers.
* **No Software Scheduler:** **avrOS** does not have a "tick" or a complex task manager. Instead, it relies on the AVR's robust **hardware interrupt controller** to handle all preemption and priority management. An interrupt triggers a vector, which is the ultimate, minimal latency "context switch."

### 2. Decentralized, Prioritized Initialization (Linker Sets)
System modularity is achieved through a **static allocation model** using custom linker sections.
* **Distributed Tables:** Developers can declare Finite State Machines (FSMs), drivers, and events in multiple, separate source files. The GCC linker automatically collects and coalesces these declarations into a single contiguous table at build time.
* **No Central "Master" List:** This "Linker Set" pattern decouples files, simplifying development and maintenance.
* **Static Initialization:** During the Initialization (Startup) phase, the kernel walks this prioritized table once, calling initialization functions to set up the hardware before any runtime code executes. This mirrors the "Configuration Table" concept of safety-critical systems like ARINC 653.

![Decentralized System Tables](./images/distributed_system_tables.png)

### 3. Purely Responsive, Event-Driven FSM
The heart of **avrOS** is a **prioritized scan loop** that moves FSMs between specialized queues.
* **Stateful Event Pipeline:** Events are managed in their own prioritized queues and exist in one of three states: **Disarmed**, **Armed**, or **Triggered**.
* **FSM Dispatcher:** The FSM kernel walks the prioritized **Ready Queue**. It executes a single, concise, non-blocking state function ("continuation") per "ready" state machine and then returns control to the dispatcher. This continues until there are no FSMs in the Ready queue. At this time, the FSM dispatcher returns to the application's main loop where it can implement sleep management via an avrOS provided API.
* **Rescheduling on Event:** When an event is triggered (e.g., from an ISR), the FSM schedules the corresponding state machine. To ensure responsiveness, if a higher-priority FSM is made ready, the dispatcher **resets to the top** of the queue, ensuring the most critical code runs next.

### 4. The "Race to Sleep" Power Model
**avrOS** prioritizes efficiency. Power consumption is directly proportional to event density.
* **"Sleep on Idle" Loop:** The main application loop is exceptionally lean. It dispatches all work until the Ready Queue is empty, then calls the processor’s `sleep` instruction.
* **Zero Polling:** The CPU does not pace, poll, or check status while idle. It sleeps, consuming minimal power, and is woken only by a hardware interrupt.
* **Developer Control:** **avrOS** exposes the `main()` loop to the developer, providing ultimate flexibility. The developer has total control over which sleep mode to use and when, allowing for precise dynamic power scaling based on application needs.

![Optimized Power Consumption](./images/power_consumption.png)

### 5. Developer-Controlled Determinism (Correctness by Construction)
Determinism in **avrOS** is not an OS variable; it is a direct reflection of application code quality.
* **Run-to-Completion:** All state functions must be concise and non-blocking. Large algorithms must be broken into "manageable chunks" that fit within a single scan cycle.
* **No Priority Inversion:** To keep the RAM footprint tiny and the code simple, **avrOS** uses fixed priority. The system relies on the developer to manage timing through task decomposition and stateful transitions.
* **Total Transparency:** This model removes all "magic" from the scheduler, providing 100% predictable execution. If a state transition must happen in a specific window, the developer has the direct visibility needed to ensure it does.

---

## avrOS Features
### System Services:
* **System** (sys) - Provides a system initialization, system timer, and sleep
* **Finite State Machine manager** (fsm) - Manages user defined state machines implementing prioritized scheduling and power management
* **Events API** (evnt) - An Asynchronous event system that device drivers and state machines can use to notify the system of various occurances. One or more state machines can wait on a single event
* **Queues API** (que) - Inter-State Machine communication mechanism. It can be used to synchronize or pass data between two or more state machines
* **Lists API** (lst) - Linked list manipulation API. Used to build and manage singly linked lists.
* **Memory Pools**[^3] (mpl) - Implements user defined heaps with dynamic fixed block sized memory allocation and free. This provides a mechanism for dynamic memory allocation that is not prone to memory fragmentation
* **Extensible Command Line Interface** (cli) - Simplifies debugging by providing a simple method to create command line "apps" that exercise or provide status on system functions
* **Logging API** (log) - Provides a mechanism to insert log messages in code this can be filtered on severity or conditionally compiled out of the application
* **Test Manager**[^3] (tst) - Unit test manager, maintains tests in groups that can be run individually, as a group, or all at once. Provides tests results and summary.
* **Alarm manager**[^3] (alrm) - Provides a mechanism for an application to implement alarms that can be acknowledged by the user
* **Modbus protocol**[^3] (mod) - Modbus RTU server and client protocol stacks to enable off-board communication using RS-232 or RS-485 serial interfaces
* **Pulse Code Modulated sound player**[^3] (pcm) - Plays PCM encoded sound converted from various sound file formats using the wav2c utility

### AVR DA device drivers:
* **General purpose I/O** (gpio) - Manipulate the AVR general purpose I/O pins
* **Universal async recevier/transmitter** (uart) - Buffered serial interface driver
* **Digital to analog converter**[^3] (dac) - Output analog values on the AVR DAC pin
* **Analog to digital converter**[^3] (adc) - Capture analog values from the AVR ADC pin(s)
* **Internal CPU oscillator** (cpu) - Configure internal clock used for the system tick
* **Memory map/stack/usage diagnostics** (mem) - Determine RAM and Flash memory usage

### Physical device drivers:
* **Button/switch**[^3] (btn) - Digital button or switch driver with de-bounce 
* **Rotary Encoder**[^3] (rot) - Rotary encoder driver
* **PCM Audio**[^3] (pcm) - PCM audio player that works with the Sound Converter Utility
* **7 segment LED display**[^3] (7seg) - Matrixed 7 Segment LED display driver
* **PS/2 keyboard interface**[^3] (ps2) - PS/2 keyboard/mouse protocol driver 

### Linux based utilities and scripts:
* **avrOS Command Center**[^3] (avrcc) - Linux text mode application to connect to avrOS applications using the CLI, logging, and/or alarm services. This application enables these services to share the same serial interface thus reducing the resources (pins) required for an application user/debug interface
* **avrOS Dash Board**[^3] (arvdb) - Example Linux graphical application using the Grafana data visualization tool, Prometheus time series database, and the avrOS modbus service
* **Installation Scripts** - Bash shell scripts that automate the installation of a fully functional AVR development system on Debian based Linux distributions such as Debian, Ubuntu, and Raspberry Pi OS. These scripts install freely available open source tools such as avr-gcc, binutils, Cppcheck static code analyzer, GNU code complexity analyzer, AVR DA family library and header files, a variety of program editors/IDEs, and avrDude. There's even a script to setup a Raspberry Pi as a headless development environment with serial connectivity and avr UPDI programming. This setup can be remotely accessed using VsCode, Zed, or even ssh from your desktop development PC
* **Sound Converter Utility** (snd2c) - Utility to convert various sound file formats to C code data structures that can be linked with user applications
* **Serial Keyboard Service** (serkey) - Linux user mode serial keyboard/HMI device daemon. It can be used with avrOS applications and others to implement HMI devices connected to a Linux device using a serial port

## Project Status and Additional Resources
avrOS is still in it's sub 1.0 development stage. So there are lots of features 
and drivers still under development.

For more information regarding avrOS, refer to the [User Manual](./doc/MANUAL.md).

For an example of Raspberry Pi 4 based development environment, see the
[Raspberry PI 4 model B Development Platform](./doc/PI4_Dev_Station.md) document.

## Install Development Environment and Build an avrOS Application
avrOS is developed on a Linux workstation oor Raspberry PI using the avr-gcc compiler,
gnu make, and avrdude. The w/Atmel ICE JTAG programmer iis optioinal. To recreate this
development environment on a debian based Linux distribution, follow the instructions here:

1. From your "Projects" directory, clone avrOS from github

    ```console
    sudo apt update
    sudo apt install git
    git clone https://github.com/racerxr650r/avrOS.git
    ```
   
   You will find these instructions in ./avrOS/README.md included in the project
   files from git. In addition, ./.vscode/c_cpp_properties.json configuration file
   is included with the project. This file will setup the visual studio code 
   C/C++ intellisense to find all the appropriate include directories and files.

2. If you prefer a manual installation, skip to the next step. Else, continue with this step to complete your intallation
   
   For an automated installation, first set the $AVROSHOME environment variable
   with the following commands
   
   ```console
   cd avrOS
   export AVROSHOME=$(pwd)
   ```

   Then run the applicable install script(s) found in the $AVROSHOME/util/scripts
   directory. The following table describes each of these scripts

   | Script               | Description                                     |
   |----------------------|-------------------------------------------------|
   | install_cli_tools.sh | Installs the required command line tools (gcc, binutils, avrdude, tio, Microchip Device Family Pack, etc.), builds the example application, and sets up git |
   | install_gui_tools.sh | Installs a set of helpful GUI development tools (geany, git-cola, meld, gtkterm, and vscode) |
   | install_all_tools.sh | Installs both the CLI and GUI tools mentioned above |
   | install_remote_pi.sh | Installs the command line tools (plus btm), sets up configurations for tio, tmux, and bash, and configures the /boot/config.txt to enable serial console and uarts 2, 3, and 4. This script should only be run on a Raspberry Pi intended for headless remote development. See the [Raspberry PI 4 model B Development Platform](./doc/PI4_Dev_Station.md) document for more details |
   | install_avrdude.sh   | Downloads, boulds, and installs avrdude from the latest version on github |
   | setup_git.sh         | Prompts and configures the username and email for git. install_cli_tools.sh calls this script |

> [!WARNING]
> These automated scripts will install additional software
software packages and possibly update config files. I encourage you to
review these scripts before running any of them.

3. Install Gnu make, git, avr-gcc Gnu C compiler, and other CLI tools

   ```console
   sudo apt install make git binutils gcc-avr avr-libc flex byacc bison unzip avrdude
   ```
    
4. Download the Microchip Device Family Pack for the Atmel-Dx series from the [Microchip Packs Repository](http://packs.download.atmel.com/)

   ```console
   wget http://packs.download.atmel.com/Atmel.AVR-Dx_DFP.2.2.253.atpack
   ```

5. Extract the `Atmel.AVR-Dx_DFP.2.4.286.atpack` file locally and copy it to the `/usr/lib/gcc/avr/5.4.0` directory

   ```console
   mkdir ./Atmel.AVR-Dx_DFP.2.4.286
   unzip -d Atmel.AVR-Dx_DFP.2.4.286/ Atmel.AVR-Dx_DFP.2.4.286.atpack
   sudo cp -R Atmel.AVR-Dx_DFP.2.4.286/ /usr/lib/gcc/avr/5.4.0
   rm -rf Atmel.AVR-Dx_DFP.2.4.286
   rm Atmel.AVR-Dx_DFP.2.4.286.atpack
   ```

6. (Optional) Install the latest AVRDUDE from sources on github

   Go to this [AVRDUDE github page](https://github.com/avrdudes/avrdude/wiki/Building-AVRDUDE-for-Linux)
   for instructions to clone, build, and install it from the latest source

> [!IMPORTANT]
> If you are using an older distribution based on Debian 10 or earlier, you may need
to do this because the version the Debian/Ubuntu repositories does not support Atmel Ice and Serial
UPDI programming interfaces. Distributions based on Debian 12 (Bookworm) will have a current version
of AVRDUDE and this step is not required

7. (Optional) Install Tio command line serial console application for the avrOS
   command line interface and logger

   ```console
   sudo apt update
   sudo apt install tio
   ```
   
   If this is on a Raspberry Pi 4 and uarts 2, 3, and 4 have been wired up as noted
   in the [Raspberry PI 4 model B Development Platform](./doc/PI4_Dev_Station.md) document,
   open a command line editor such as micro with the ~/.tioconfig file and add
   the following lines

   ```console
   # Defaults
   baudrate = 115200
   databits = 8
   parity = none
   stopbits = 1

   [cli]
   color = 2
   device = /dev/ttyAMA3

   [log]
   color = 3
   device = /dev/ttyAMA4
   ```

   After saving the .tioconfig file to your home directory, you can enter the
   following commands at the command line

   ```console
   tio cli
   ```
   or
   ```console
   tio log
   ```

   to connect to the avrOS command line and logger respectively

8. Build avrOS example application

   Goto the application directory and make the .hex image

   ```console
   cd avrOS/app/avrOS_example
   make
   ```

9. Program the .elf image into the MCU flash [^1] [^2]

   ```console
   make flash
   ```
    
    The default programmer defined in the makefile is the /dev/ttyAMA2 serial
    port using serialupdi. If you are not running on a Pi 4 or 5, change this
    to the appropriate serial port or the atmel ice (atmelice_updi)

10. Setup your git username and email

    If you haven't already, you should setup your git username and email address
    using the following commands.

    To setup your username:

    ```console
    git config --global user.name "John Doe"
    ```

    To setup your email:

    ```console
    git config --global user.email "john_doe@gmail.com"
    ```
    
These instructions are similar for Fedora and MacOS. You'll need to use the
appropiate package manager.

[^1]: The make flash target will build and program the application into flash
[^2]: If you are using a different programmer that is supported by AVRDUDE, 
change PRG in the makefile to the string AVRDUDE uses for your programmer
[^3]: This is a planned future feature
