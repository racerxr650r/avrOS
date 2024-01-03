/*
 * gpio.c
 *
 * Implement general purpose input output
 *
 * Created: 11/10/2023 6:59:29 PM
 * Author: john anderson
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

// Externs --------------------------------------------------------------------
extern void *__start_GPIO_TABLE,*__stop_GPIO_TABLE;

// Internal Function Prototypes -----------------------------------------------
static void isrInput(PORT_t *port);

// Interrupt Table Hooks ------------------------------------------------------
// Hook the IO Port A interrupt
ISR(PORTA_PORT_vect)
{
	isrInput(&PORTA);
}

// Hook the IO Port C interrupt
ISR(PORTC_PORT_vect)
{
	isrInput(&PORTC);
}

// Hook the IO Port D interrupt
ISR(PORTD_PORT_vect)
{
	isrInput(&PORTD);
}

// Hook the IO Port F interrupt
ISR(PORTF_PORT_vect)
{
	isrInput(&PORTF);
}

// Port Interrupt Handler
static void isrInput(PORT_t *port)
{
	// Walk the gpio table
	gpio_t *gpio = (gpio_t *)&__start_GPIO_TABLE;
	for(; gpio < (gpio_t *)&__stop_GPIO_TABLE; ++gpio)
	{
		// If this is the same port...
		if(gpio->port == port)
		{
			// If this is the same pin..
			if(gpio->pin && gpio->port->INTFLAGS)
			{
				// Call the registered handler
				gpio->handler(gpio);
				// Clear the interrupt flag
				gpio->port->INTFLAGS = gpio->pin;
				// Return from the interrupt
				break;
			}
		}
	}
}

// Command Line Interface -----------------------------------------------------
#ifdef GPIO_CLI
ADD_COMMAND("gpio",gpioCmd,true);
#endif

static int gpioCmd(int argc, char *argv[])
{
	int ret = -1;

	// Walk the gpio table
	gpio_t *gpio = (gpio_t *)&__start_GPIO_TABLE;
	for(; gpio < (gpio_t *)&__stop_GPIO_TABLE; ++gpio)
	{
		char port = gpio->port==&PORTA?'A':gpio->port==&PORTC?'C':gpio->port==&PORTD?'D':gpio->port==&PORTF?'f':'?';
		char *direction = gpio->direction==GPIO_OUTPUT?"Out":gpio->direction==GPIO_INPUT?"In":"Unknown";
		char *interrupt = gpio->handler==NULL?"No":"Yes";
		
		if(argc<2 || (argc==2 && !strcmp(gpio->name,argv[1])))
		{
			printf(BOLD FG_BLUE UNDERLINE "%-16s",gpio->name);
			printf(" port: %c pins: 0x%02x direction: %3s interrupt: %3s\n\r" RESET,port,gpio->pin,direction,interrupt);

			if(gpio->direction == GPIO_OUTPUT)
				printf("\tvalue: 0x%02x\n\r",gpioReadOutput(gpio));
			else
				printf("\tvalue: 0x%02x\n\r",gpioReadInput(gpio));

#ifdef GPIO_STATS
			//printf("\tStuff\n\r");
#endif
			ret = 0;
		}
	}
	return(ret);
}

#ifdef GPIO_CLI
ADD_COMMAND("gpioSet",gpioSetCmd);
#endif

static int gpioSetCmd(int argc, char *argv[])
{
	int ret = -1;

	if(argc==2)
	{
		// Walk the gpio table
		gpio_t *gpio = (gpio_t *)&__start_GPIO_TABLE;
		for(; gpio < (gpio_t *)&__stop_GPIO_TABLE; ++gpio)
		{
			if(!strcmp(gpio->name,argv[1]))
			{
				gpioSetOutput(gpio,gpio->pin);
				ret = 0;
			}
		}

	}
	return(ret);
}

#ifdef GPIO_CLI
ADD_COMMAND("gpioClr",gpioClrCmd);
#endif

static int gpioClrCmd(int argc, char *argv[])
{
	int ret = -1;

	if(argc==2)
	{
		// Walk the gpio table
		gpio_t *gpio = (gpio_t *)&__start_GPIO_TABLE;
		for(; gpio < (gpio_t *)&__stop_GPIO_TABLE; ++gpio)
		{
			if(!strcmp(gpio->name,argv[1]))
			{
				gpioClearOutput(gpio,gpio->pin);
				ret = 0;
			}
		}

	}
	return(ret);
}

#ifdef GPIO_CLI
ADD_COMMAND("gpioTgl",gpioTglCmd);
#endif

static int gpioTglCmd(int argc, char *argv[])
{
	int ret = -1;

	if(argc==2)
	{
		// Walk the gpio table
		gpio_t *gpio = (gpio_t *)&__start_GPIO_TABLE;
		for(; gpio < (gpio_t *)&__stop_GPIO_TABLE; ++gpio)
		{
			if(!strcmp(gpio->name,argv[1]))
			{
				gpioToggleOutput(gpio,gpio->pin);
				ret = 0;
			}
		}

	}
	return(ret);
}

#ifdef GPIO_CLI
ADD_COMMAND("gpioWrOut",gpioWrCmd);
#endif

static int gpioWrCmd(int argc, char *argv[])
{
	int ret = -1;

	if(argc==3)
	{
		// Walk the gpio table
		gpio_t *gpio = (gpio_t *)&__start_GPIO_TABLE;
		for(; gpio < (gpio_t *)&__stop_GPIO_TABLE; ++gpio)
		{
			if(!strcmp(gpio->name,argv[1]))
			{
				gpioWriteOutput(gpio,(uint8_t)atoi(argv[2]));
				ret = 0;
			}
		}

	}
	return(ret);
}

#ifdef GPIO_CLI
ADD_COMMAND("gpioRdIn",gpioRdInCmd,true);
#endif

static int gpioRdInCmd(int argc, char *argv[])
{
	int ret = -1;

	if(argc==2)
	{
		// Walk the gpio table
		gpio_t *gpio = (gpio_t *)&__start_GPIO_TABLE;
		for(; gpio < (gpio_t *)&__stop_GPIO_TABLE; ++gpio)
		{
			if(!strcmp(gpio->name,argv[1]))
			{
				printf("\n\rValue: 0x%02x\n\r",gpioReadInput(gpio));
				ret = 0;
			}
		}

	}
	return(ret);
}

#ifdef GPIO_CLI
ADD_COMMAND("gpioRdOut",gpioRdOutCmd);
#endif

static int gpioRdOutCmd(int argc, char *argv[])
{
	int ret = -1;

	if(argc==2)
	{
		// Walk the gpio table
		gpio_t *gpio = (gpio_t *)&__start_GPIO_TABLE;
		for(; gpio < (gpio_t *)&__stop_GPIO_TABLE; ++gpio)
		{
			if(!strcmp(gpio->name,argv[1]))
			{
				printf("\n\rValue: 0x%02x\n\r",gpioReadOutput(gpio));
				ret = 0;
			}
		}

	}
	return(ret);
}
// Internal functions -------------------------------------------------------------

// Initialize a given gpio port to the given parameters
int gpioInit(const fsmStateMachineDescr_t *stateMachineDescr)
{
	gpio_t *gpioInstance = (gpio_t *)initGetInstance(stateMachineDescr);

//	INFO("Init GPIO");

#ifdef GPIO_STATS
	// Zero out the interface stats
	memset((void *)&gpioInstance->stats,0,sizeof(gpioStats_t));
#endif	

	// If this gpio is an output...
	if(gpioInstance->direction == GPIO_OUTPUT)
		gpioInstance->port->DIRSET = gpioInstance->pin;
	// Else this gpio is an input...
	else
	{
		gpioInstance->port->DIRCLR = gpioInstance->pin;
		gpioInstance->port->PINCONFIG = PORT_PULLUPEN_bm;
		gpioInstance->port->PINCTRLUPD = gpioInstance->pin;
	}

	// If this gpio has an interrupt handler...
	if(gpioInstance->handler != NULL)
	{
		gpioInstance->port->PINCONFIG = PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;
		gpioInstance->port->PINCTRLUPD = gpioInstance->pin;
	}
	
	return(0);
}

// External functions ---------------------------------------------------------
void gpioSetOutput(const gpio_t *gpio, uint8_t value)
{
	gpio->port->OUTSET = (value & gpio->pin);
}

void gpioClearOutput(const gpio_t *gpio, uint8_t value)
{
	gpio->port->OUTCLR = (value & gpio->pin);
}

void gpioToggleOutput(const gpio_t *gpio, uint8_t value)
{
	gpio->port->OUTTGL = (value & gpio->pin);
}

void gpioWriteOutput(const gpio_t *gpio, uint8_t value)
{
	uint8_t tmp = gpio->port->OUT;

	value &= gpio->pin;
	tmp &= ~gpio->pin;
	gpio->port->OUT = value | tmp; 
}

uint8_t gpioReadInput(const gpio_t *gpio)
{
	return(gpio->port->IN & gpio->pin);
}

uint8_t gpioReadOutput(const gpio_t *gpio)
{
	return(gpio->port->OUT & gpio->pin);
}
