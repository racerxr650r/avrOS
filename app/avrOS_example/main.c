/*
 * main.c
 *
 * Application OS resources, state machine(s), CLI commands, and main entry 
 * point. The main entry point calls sysInit(), enables interrupts, and then 
 * loops forever calling the fsmDispatch() function that implements the state
 * machine scheduler.
 *
 * Created: 2/8/2021 12:39:38 PM
 * Author : john anderson
 *
 * Copyright (C) 2021 by John Anderson <racerxr650r@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any 
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */ 

#include "avrOS.h"

// AVR Fuse configuration -----------------------------------------------------
FUSES =
{
	.WDTCFG = FUSE_WDTCFG_DEFAULT,		///< Default
	.BODCFG = FUSE_BODCFG_DEFAULT,		///< Default
	.OSCCFG = FUSE_OSCCFG_DEFAULT,		///< Default
	.SYSCFG0 = 0xCC,					///< External Reset enabled on PF6
	.SYSCFG1 = FUSE_SYSCFG1_DEFAULT,	///< Default
	.CODESIZE = FUSE_CODESIZE_DEFAULT,	///< Default
	.BOOTSIZE = FUSE_BOOTSIZE_DEFAULT	///< Default
};

// AVR Lock bits configuration ------------------------------------------------
LOCKBITS = LOCKBITS_DEFAULT;

// Internal function prototypes -----------------------------------------------
void btnHandler(gpio_t *gpio);

// Logger Configuration -------------------------------------------------------
// Adding this first so the logger is available for other device initializers
ADD_UART_WRITE(logUart,LOG_USART,LOG_BAUDRATE, LOG_PARITY, LOG_DATA_BITS, LOG_STOP_BITS, LOG_QUEUE_SIZE);
ADD_LOG(logger,UART_FILE_PTR(logUart));

// Command Line Interface Configuration ----------------------------------------
//ADD_UART_RW(cliUart, CLI_USART, CLI_BAUDRATE, CLI_PARITY, CLI_DATA_BITS, CLI_STOP_BITS, CLI_TX_QUEUE_SIZE, CLI_RX_QUEUE_SIZE);
//ADD_CLI(command_line,UART_FILE_PTR(cliUart));

// GPIO Configuration ---------------------------------------------------------
ADD_GPIO(Sleep_gpio,PORTD,GPIO_PIN_0,GPIO_OUTPUT);
ADD_GPIO(Leds_gpio,PORTD,GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,GPIO_OUTPUT);
//ADD_GPIO(Green,PORTD,GPIO_PIN_1,GPIO_OUTPUT);
//ADD_GPIO(Yellow,PORTD,GPIO_PIN_2,GPIO_OUTPUT);
//ADD_GPIO(Red,PORTD,GPIO_PIN_3,GPIO_OUTPUT);

ADD_GPIO(Clock,PORTA,GPIO_PIN_0,GPIO_INPUT);
ADD_GPIO(Data,PORTA,GPIO_PIN_1,GPIO_INPUT);
ADD_GPIO(Button,PORTA,GPIO_PIN_2,GPIO_INPUT,btnHandler);

// State Machine Configuration ------------------------------------------------
ADD_STATE_MACHINE(Leds_sm,ledsInit, FSM_APP | 10);

int ledsInit(volatile fsmStateMachine_t *stateMachine)
{
	static uint8_t ledsCounter = 0;
	static uint8_t pattern[] = {0b00001110, 0b00001100, 0b00001010, 0b00000110, 0b00000000};
	
	if(++ledsCounter == sizeof(pattern))
		ledsCounter = 0;

	gpioWriteOutput(&Leds_gpio,pattern[ledsCounter]);
	fsmWaitTicks(stateMachine, 250);

	return(0);
}

// Application entry point and system loop ------------------------------------
int main(void)
{
	// Initialize the system --------------------------------------------------
	sysInit();
	
	// Loop forever -----------------------------------------------------------
    while (1) 
    {
	    // Call the main state machine dispatcher
		fsmDispatch();
		// Go to sleep until the next interrupt
		gpioClearOutput(&Sleep_gpio,0xff);
		sysSleep();
		gpioSetOutput(&Sleep_gpio,0xff);
    }
}

void btnHandler(gpio_t *gpio)
{
	INFO("Button status %d",gpioReadInput(gpio)>>2);
}

