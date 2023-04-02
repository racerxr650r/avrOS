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
microcontrollers like the AVR. The AVR DA family has the largest RAM of the AVR
family of processors. But it only has 16K. Therefore, avrOS was designed to use
as little RAM as possible.

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
application and system services register a state machine and a set of states.
The scheduler uses a table of state machines and states to determine which
to call next. The constant data in these tables is stored in FLASH. Only the 
dynamic state information is stored in RAM. 

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