[![avrOS](doc/avrOS.gif "avrOS")](https://github.com/racerxr650r/avrOS)
---
# Getting Started

## avrOS - _The Operating System for AVR DA microcontrollers_
**avrOS** is a scalable operating system 
with drivers for the AVR DA family of microcontrollers. It provides macros and a 
custom linker script to build the various system tables implementing state 
machines, queues, events, memory heaps, command line commands, alarms, and modus 
registers at compile time. These tables reside in FLASH where possible. So the 
system doesn't require run-time registration and related fault handling code. 
In addtion, there is no need to maintain a single source file containing all 
these system tables. The macros that build these table can be distributed across
several source files so they can be co-located with the associated logic.

**avrOS** provides a state machine manager. The developer defines one or more
state machines that implement the system functionality. The state machine
manager handles priortized scheduling of these state machine states and power 
management when the system is idle waiting for asynchronous events. This
state machine approach reduces the RAM requirements for applications by
using just one stack for all of the system "processes". This differs from other
real time operating systems that use threads or tasks that require individually
reserved memory stacks in RAM. This partitioning of the system stack is
complex, inefficient, and prone to issues that are difficult to debug.

In addtion, **avrOS** provides event and queue services that enable inter-state
machine and device driver communication and syncronization. This creates a
system that is interrupt/event driven and takes advantage of the AVR DS's rich
number of interrupt sources. This reduces CPU intensive polling and makes the
built in power management even more efficient.

Lastly, the **avrOS** ecosystem also provides instructions, makefiles, and scripts to 
setup a development environment and build applications using the Linux operating
system and it's abundant open source development software and hardware resources.
You no longer need to use Microsoft Windows for AVR application development. But
if you prefer Windows on your desktop PC, you can setup a headless Raspberry Pi
for remote development using VsCode, Zed, or ssh with your favorite text mode editor.

## avrOS Features
### System Services:
* **System** (sys) - Provides a system initialization, system timer, and sleep
* **Finite State Machine manager** (fsm) - Manages user defined state machines implementing prioritized scheduling and power management
* **Queues API** (que) - Inter-State Machine communication mechanism. It can be used to synchronize or pass data between two or more state machines
* **Events API** (evnt) - An Asynchronous event system that device drivers and state machines can use to notify the system of various occurances. One or more state machines can wait on a single event
* **Memory Pools**[^3] (mpl) - Implements user defined heaps with dynamic fixed block sized memory allocation and free. This provides a mechanism for dynamic memory allocation that is not prone to memory fragmentation
* **Extensible Command Line Interface** (cli) - Simplifies debugging by providing a simple method to create command line "apps" that exercise or provide status on system functions
* **Logging API** (log) - Provides a mechanism to insert log messages in code this can be filtered on severity or conditionally compiled out of the application
* **Alarm manager**[^3] (alrm) - Provides a mechanism for an application to implement alarms that can be acknowledged by the user
* **Modbus protocol**[^3] (mod) - Modbus RTU server and client protocol stacks to enable off-board communication using RS-232 or RS-485 serial interfaces
* **Pulse Code Modulated sound player** (pcm) - Plays PCM encoded sound converted from various sound file formats using the wav2c utility

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
* **Serial Keyboard Service** (serkey) - Linux user mode serial keyboard/HMI device service. It can be used with avrOS applications to implement HMI devices connected to a Linux device using a serial port

## Project Status and Additional Resources
avrOS is still in it's sub 1.0 development stage. So there are lots of new 
features and drivers coming. For more information regarding avrOS, refer to
the [User Manual](./doc/MANUAL.md).

For an example of Raspberry Pi 4 based development environment, see the
[Raspberry PI 4 model B Development Platform](./doc/PI4_Dev_Station.md) document.

## Install Development Environment and Build an avrOS Application
avrOS is developed on a Linux workstation using the avr-gcc compiler, gnu make,
and avrdude w/Atmel Ice jtag programmer. To recreate this development 
environment on a debian based Linux distribution follow the instructions here:

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

9. Program the .hex image into the MCU flash [^1] [^2]

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
