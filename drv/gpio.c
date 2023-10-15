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

static void isrInput(PORT_t *port)
{
    // Walk the gpio table
    gpio_t *gpio = (gpio_t *)&__start_GPIO_TABLE;
    for(; gpio < (gpio_t *)&__stop_GPIO_TABLE; ++gpio)
    {
        
    }
}

// Command Line Interface -----------------------------------------------------
#ifdef GPIO_CLI
ADD_COMMAND("gpio",gpioCmd,true);
#endif

int gpioCmd(int argc, char *argv[])
{
    // Walk the gpio table
    gpio_t *gpio = (gpio_t *)&__start_GPIO_TABLE;
    for(; gpio < (gpio_t *)&__stop_GPIO_TABLE; ++gpio)
    {
        char * name;
        
        name = gpio->name;
        if(argc<2 || (argc==2 && !strcmp(name,argv[1])))
        {
            printf("\t%s \n\r",name);

#ifdef GPIO_STATS
            printf("\tTx:%8lu Rx:%8lu Frame Err:%8lu Parity Err:%8lu\n\r",uart->stats->txBytes,uart->stats->rxBytes,uart->stats->frameError,uart->stats->parityError);
            printf("\tTxOvrflw:%8lu RxBufferOvrflw:%8lu RxQueueOvrflw:%8lu\n\r",uart->stats->txQueueOverflow,uart->stats->rxBufferOverflow,uart->stats->rxQueueOverflow);
#endif
        }
    }
    return(0);
}

// State Machine Functions ----------------------------------------------------

// Initialize a given gpio port to the given parameters
int gpioInit(volatile fsmStateMachine_t *stateMachine)
{
    gpio_t *gpioInstance = (gpio_t *)fsmGetInstance(stateMachine);

#ifdef GPIO_STATS
    // Zero out the interface stats
    memset((void *)&gpioInstance->stats,0,sizeof(gpioStats_t));
#endif	

    // Start critical section of code
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if(gpioInstance->direction == GPIO_OUTPUT)
            gpioInstance->port->DIRSET = gpioInstance->pin;
        else
        {
            gpioInstance->port->DIRCLR = gpioInstance->pin;
            gpioInstance->port->PINCONFIG = PORT_PULLUPEN_bm;
            gpioInstance->port->PINCTRLUPD = gpioInstance->pin;
        }
            
    }
    
    // Enable global interrupts
    sei();
    
    // Stop the statemachine upon completion of initialization
    fsmStop(stateMachine);	
    return(0);
}
