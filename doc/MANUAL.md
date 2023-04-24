[![avrOS](./avrOS.gif "avrOS")](https://github.com/racerxr650r/avrOS)
---
# User Manual

**avrOS** - _Operating Environment for AVR DA_, is a scalable operating environment 
including drivers for the AVR DA family of microcontrollers. It uses macros, a 
custom linker script, and the linker to build the system tables (state machines,
states, drivers, services, CLI callbacks, Flags, Queues, and Timers) at compile/link 
time. There is no need to edit a single source file containing all these system
tables. Application code defining these objects can be distributed across several
source files. In addition, these tables reside in FLASH where possible and the
system does not require run-time registration and related fault handling code.

## Scarce Microcontroller Resources

RAM is a precious commodity on microcontrollers. Especially for 8 bit 
microcontrollers like the AVR. The AVR DA family only has 16K of RAM. 
Therefore, avrOS is designed to use as little RAM as possible.

Classic realtime operating systems use threaded multitasking. The OS has a 
scheduler that controls which thread is currently running. The scheduler uses a
thread priority value to determine which thread runs next. To implement the 
thread context, each thread has it's own stack. The stack is stored in RAM and
is used for passing and returning values and storing local variables for 
functions. This same stack also stores the context for any interrupts that 
happen during the thread execution. The scheduler then points the CPU stack
register to the scheduled thread stack to implement a context switch. With 
multiple threads, this requires reserving enough RAM for the deepest call stack
plus the largest interrupt context for each thread. This is not the efficient
use of RAM. A more efficient approach would use a single stack for all 
"threads".

Another feature of classic realtime operating systems is a modular design that 
organizes the system code into functional blocks. The application code
interacts using an API that declares and defines the objects these functional
blocks implement. To abstract the data structures that represent the instance
of an object and prevent the developer from having to edit system source files
containing arrays of these structures, classic operating systems dynamically
allocate memory at runtime to store these arrays of data structures. In
practice, significant portions of these data structures are populated with
constant values. Reading constant data from ROM/Flash memory to initialize an
object at runtime requires the functional block to allocate memory from RAM.
This is another inefficient use of RAM. It also requires additional code to
test and handle the condition when not enough RAM is available.

## avrOS Theory of Operation

To use RAM as efficiently as possible, avrOS implements a form of cooperative
multitasking. This requires that the application code does not block or busy
wait. Instead it will check the status of various variables or objects to 
detemine if it should do something, do it, and then return. By doing this,
avrOS is able to use a single stack for all the system threads and interrupt 
contexts.

avrOS does not implement threading. The AVR microcontrollers have a very rich
set of interrupts to handle asynchronous events. Handlers for these interrupts
are "scheduled" asynchronously by the AVR interrupt controller. avrOS objects
such as flags and queues can be used by the handler to signal and pass data to
the user application code.

In addition, the avrOS scheduler uses a finite state machine paradigm. The user
application and system services register state machines and a set of states.
The finite state machine manager (fsm) provides an API for the developer to
control the state progression of the state machine. The scheduler uses a table
of state machines and states to determine which to call next. The constant data
in these tables is stored in FLASH. Only the dynamic state information is
stored in RAM. 

These tables are built at compile time and the linker determines that there is
enough FLASH and RAM to store them. Therefore, there is no need for user code
to call APIs to create objects at runtime and include additional code to handle
conditions when there is not enough RAM to create a new object.

To enable this feature and maintain an object oriented approach to software
development, avrOS provides a set of macros for user code to define system
objects. These macros "allocate" instances of state machines, states, queues,
flags, timers, CLI commands, alarms, etc. at compile time and stores much of
the data in flash where it will stay at runtime.

Lastly, avrOS is highly scalable. Using the avrOSConfig.h file, an application
developer can select precisely the features and drivers required by their
implementation. For instance, a debug version of application may include the
CLI and Logger services. But, the release version of the same application may
not include either of these services.

## avrOS File Organization

avrOS is organized into 7 directories counting the root directory; ./, ./app,
./docs, ./drv, ./srv, ./sys, and ./util

```console
.
├── app
│   └── avrOS_example
│       ├── avrOSConfig.h
│       ├── avrOS.x
│       ├── main.c
│       └── makefile
├── doc
│   ├── avrOS.gif
│   ├── avrOS.xcf
│   └── MANUAL.md
├── drv
│   ├── cpu.c
│   ├── cpu.h
│   ├── dac.c
│   ├── dac.h
│   ├── mem.c
│   ├── mem.h
│   ├── uart.c
│   └── uart.h
├── srv
│   ├── cli.c
│   ├── cli.h
│   ├── log.c
│   ├── log.h
│   └── pcm.c
├── sys
│   ├── fsm.c
│   ├── fsm.h
│   ├── queue.c
│   ├── queue.h
│   ├── sys.c
│   ├── sys.h
│   ├── tmr.c
│   └── tmr.h
├── util
|   └── wav2c
|       └── wav2c.c
├── avrOS.h
├── LICENSE
└── README.md
```
Root contains the avrOS.h header file. 

**./app/avrOS_example** contains the makefile, avrOSConfig.h, main.c, and avrOS.x
files. The avrOSConfig.h file selects the components to be included in the
build. The avrOS.x file is a linker script. The main.c file contains the main()
entry point and the user application state machine and system objects.

**./doc** contains relavent documents and graphic files. This includes the document
you are reading now and the avrOS logo graphic files.

**./drv** contains the device drivers.

**./srv** contains the system services such as the CLI manager and Logging.

**./sys** contains the source files that implement system initialization, the
finite state machine manager, and the OS objects (flags and queues). 

**./util** contains host utility programs.

## avrOS Application Directory

The ./app directory contains a sub-directory for each application. These
directories should contain at least the following files.

**avrConfig.h** configures the system, driver, and service files to be
included with your application. It's self documenting with a signficant
number of comments included in the file.

**avrOS.x** is the linker script for your application. avrOS is dependent
on this linker file. Do not replace it with a standard linker script without
updating it to include the required sections and symbols. For more
information regarding this, see the *avrOS Linker Script* page in the avrOS
wiki.

**main.c** source file contains the entry point `main()` for your application.
Main calls sysInit() to perform the runtime initialization of avrOS. It then
enters an endless while loop calling fsmDispatch() and sysSleep(). This
function implements the finite state machine scheduler. This scheduler walks
the state machine and state tables to determine which state to run. The
scheduler will continue to call states until the "ready" queue is empty. At
that time, it will return. The loop in main() then calls sysSleep(). This
function puts the processor into a sleep state and stops execution. Execution
will resume and sysSleep() will return once an external interrupt is triggered.
The loop then repeats.

**makefile** is the make script to build, clean, and flash your application.

## Building avrOS Application

An application is built from the ./app/application_name directory. avrOS
comes with a ./app/avrOS_example directory and application code example.
`make all` from the command line in the application directory will build the
application .hex file. When building the application, the makefile creates a
./build directory in the application directory. It's here you will find the
.hex file and the other generated object files.

The `make clean` command will delete the ./build directory and it's contents

To create your own application, make a new directory in ./app directory. Then,
copy the makefile, avrOS.x, avrConfig.h, and main.c files from the
./app/avrOS_example to your new directory. 

The makefile will build your application without any changes. If you choose to
rename main.c, you will have to modify the PRJ variable in the makefile to the
same name of your renamed main.c file excluding the .c file extension.


avrOS also comes with a makefile and instructions to setup a development
environment and build applications on a Linux desktop PC, chromebook, or even a
Raspberry PI. There's no need to use Atmel Studio and Windows for AVR application
development.

## Loading and Running avrOS Application

The makefile uses [AVRDUDE](https://github.com/avrdudes/avrdude/wiki/Building-AVRDUDE-for-Linux)
to program the target CPU. By default, it uses the Atmel Ice as the programmer.
To change this, modify the PRG variable in the makefile.

```
# avr programmer (and port if necessary)
# e.g. PRG = atmelice_updi -or- PRG = serialupdi -P /dev/ttyUSB0
# The current programmer if the Atmel ICE w/UPDI interface
PRG = atmelice_updi
```

`make flash` will program the application .hex file into the AVR flash program
memory and reset the processor.

`make test` will test the connectivity to the programmer.

## System

avrOS provides the following system objects and functions:

* Finite State Machine manager (fsm)
* Memory usage API (mem)
* Flags API (flg)
* Queues API (que)
* Timers API (tmr)

## Services

avrOS provides the following system services:

* Command Line Interface (cli)
* Logger (log)
* Pulse Code Modulated sound player API (pcm)

## Drivers

avrOS provides the following AVR DA device drivers:

* UART
* System Tick (16 bit Timer Type B)
* DAC
* Internal CPU Oscillator API

## Utilities

avrOS includes a Linux command line utility `wav2c` to convert a 
number of sound and video file formats to a C file that can be linked with
your application and played with the PCM sound player API.
