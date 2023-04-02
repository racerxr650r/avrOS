# avrOS

avrOS - Operating Environment for AVR DA - is a scalable operating environment 
including drivers for the AVR DA family of microcontrollers. It provides the
following features:

* Finite State Machine manager (fsm)
* Command Line Interface (cli)
* Logger (log)
* Memory usage API (mem)
* Pulse Code Modulated sound player API (pcm)
* Flags API (flg)
* Queues API (que)
* Timers API (tmr)

In addition, it also includes the following AVR DA device drivers:

* UART
* System Tick (16 bit Timer Type B)
* DAC
* Internal CPU Oscillator API

Lastly, it also includes a Linux command line utility `wav2c` to convert a 
number of sound and video file formats to a C file that can be linked with
your application and played with the PCM sound player API.

avrOS is still in it's early development stage. So there are lots of new 
features and drivers coming.

## Microcontroller Resources

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
plus the largest interrupt context for each thread. This is not the most 
efficient use of RAM. A more efficient approach would use a single stack for 
all "threads".

Another feature of classic realtime operating systems is a modular design that 
organizes code into functional blocks. Each block is represents a black box. 
The external software that uses these blocks interacts with the black box 
through a defined API. To abstract the data structures that represent the 
instance of an object a block implements, classic operating systems dynamically
allocate memory to store these data structures. In practice, significant 
portions of these data structures are populated with constant values. Reading 
constant values from ROM/Flash memory to initialize an object at runtime
requires the functional block to allocate memory from RAM. This is another
inefficient use of RAM. It also requires additional code to test and handle the
condition when not enough RAM is available.

## Theory of Operation

To use RAM as efficiently as possible, avrOS implements a type of cooperative
multitasking. This requires that the application code does not block or busy
wait. Instead it will check the status of various variables or objects to 
detemine if it should do something, do it, and then return. By doing this,
avrOS is able to use a single stack for all the system threads and interrupt 
contexts.

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
development, avrOS provides a set of macros for user code to declare objects.
These macros allocate instances of state machines, states, queues, flags,
timers, CLI commands, alarms, etc. at compile time.

Lastly, avrOS is highly scalable. Using the avrOSConfig.h file, an application
developer can select precisely the features and drivers required by their
implementation. For instance, a debug version of application may include the
CLI and Logger functions. But, the release version of the same application may
not include either of these functions.

## Development Environment

avrOS is developed on a Linux workstation using the avr-gcc compiler, gnu make,
and avrdude w/Atmel Ice jtag programmer. To recreate this development 
environment on a debian based Linux distribution follow the instructions here:

1. Install Gnu make

    ```console    
    sudo apt install make
    ```

2. Install avr-gcc Gnu C compiler and tools

    ```console
    sudo apt install binutils gcc-avr avr-libc flex byacc bison
    ```

3. Download the Microchip Device Family Pack for the Atmel-Dx series from the [Microchip site](http://packs.download.atmel.com/)

4. Extract the `Atmel.AVR-Dx_DFP.2.2.253.atpack` file locally and copy it to the `/usr/lib/gcc/avr/5.4.0` directory
From the directory you extracted the DFP use the following command to copy it:

    ```console
    sudo cp -R Atmel.AVR-Dx_DFP.2.2.253/ /usr/lib/gcc/avr/5.4.0
    ```

    If your file manager does not recognize it as a compressed file, add a .zip extension to the filename   
    If you extracted/copied it elsewhere, you will need to update DFP to the path of the base directory you just extracted

5. Install AVRDUDE from sources on github

    Go to this [AVRDUDE github page](https://github.com/avrdudes/avrdude/wiki/Building-AVRDUDE-for-Linux)
    for instructions to clone, build, and install it from the latest source

    I recomend this version because the version in the Debian/Ubuntu package
    repository does not include support for the Atmel Ice and it's UPDI programming
    interface.

6. Clone avrOS from github

    ```console
    git clone https://github.com/racerxr650r/avrOS.git
    ```

7. Build avrOS example application

    Goto the application directory and make the .hex image

    ```console
    cd avrOS/app/avrOS_example
    make
    ```

8. Program the .hex image into the MCU flash [^1] [^2]

    ```console
    make flash
    ```

[^1]: The make flash target will build and program the application into flash
[^2]: If you are using a different programmer that is supported by AVRDUDE, 
change PRG in the makefile to the string AVRDUDE uses for your programmer