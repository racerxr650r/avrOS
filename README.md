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

Lastly, it also includes a Linux command line `wav2c` utility to convert
a number of sound and video file formats to a C file that can be linked
with your application and played with the PCM sound player API.

