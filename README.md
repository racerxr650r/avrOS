[![avrOS](doc/avrOS.gif "avrOS")](https://github.com/racerxr650r/avrOS)
---
# avrOS Getting Started

**avrOS** - _Operating System for AVR DA_, is a scalable operating system 
with drivers for the AVR DA family of microcontrollers. It provides macros and a 
custom linker script to build the system tables (state machines, drivers (GPIO,
UART, etc.), services, command line commands, events, and queues) at compile time. 
Source code defining these objects can be distributed across several source files.
In addition, these tables reside in FLASH where possible. So the system doesn't
require run-time registration and related fault handling code. There is no need
to edit a single source file containing all these system tables.

avrOS also provides a makefile and instructions to setup a development
environment and build applications on a Linux desktop PC, chromebook, or even a
Raspberry PI. You can leave Windows behind for AVR application development.

avrOS provides the following system objects and services...

* System Tick _Timer Type B_ (sys)
* Finite State Machine manager (fsm)
* Extensible Command Line Interface (cli)
* Logger (log)
* Pulse Code Modulated sound player API (pcm)
* Queues API (que)
* Events (evnt)

...the following AVR DA device drivers...

* General purpose I/O (gpio)
* Universal async recevier/transmitter (uart)
* Digital to analog conveter (dac)
* Internal CPU oscillator (cpu)
* Memory map/stack/usage diagnostics (mem)

...and the following generic device drivers...

* Button/switch (btn)
* Rotary Encoder (rot)
* PCM Audio (pcm)
* 7 segment LED display (7seg)
* PS/2 keyboard interface (ps2)

Lastly, it also includes a Linux scripts to install command line and GUI 
development tools required to build avrOS applications. There is also a
command line utility `wav2c` to convert a number of sound and video file
formats to a C file that can be linked with your application and played 
with the PCM sound player API.

avrOS is still in it's sub 1.0 development stage. So there are lots of new 
features and drivers coming. For more information regarding avrOS, refer to
the [User Manual](./doc/MANUAL.md).

For an example of Raspberry Pi 4 based development environment, see the
[Raspberry PI 4 model B Development Platform](./doc/PI4_Dev_Station.md) document.

## Install Development Environment and Build

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

2. If you prefer a manual installation, skip to the next step.
   
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

   > :memo: **Note:** These automated scripts will install additional software
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

   > :memo: **Note:** If you are using an older distribution based on Debian 10 or earlier, you may need
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
