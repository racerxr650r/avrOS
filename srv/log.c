/*
 * log.c
 *
 * Created: 4/25/2021 6:01:41 PM
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
 // Includes -------------------------------------------------------------------
 #include "../avrOS.h"

// Private Globals ------------------------------------------------------------
static FILE	logSTDOUT = FDEV_SETUP_STREAM(uartPutChar, NULL, _FDEV_SETUP_WRITE);

// OS Objects -----------------------------------------------------------------
#ifdef LOG_SERIAL
// Create the Tx queue
ADD_QUEUE(logQueue,LOG_QUEUE_SIZE);
// Create the Uart instance for CLI I/O
ADD_UART(logUart,LOG_USART,&logQueue,NULL);
#endif

// External Functions ---------------------------------------------------------
// Initialize the CLI and the associated serial port
void logInit()
{
	// Setup stderr so stdout.h functions like fprintf output the log associated
	// uart or virtual uart
	fdev_set_udata(&logSTDOUT,(void *)&logUart);
	stderr = &logSTDOUT;

#ifdef LOG_SERIAL
	// Initialize CLI UART
	uartInit(&logUart,LOG_BAUDRATE,LOG_PARITY,LOG_DATA_BITS,LOG_STOP_BITS);
#endif

	// Enable global interrupts
	sei();

	// Hide the cursor
	fprintf(stderr,"\e[?25l");

	// Display the system greeting
	fprintf(stderr,LOG_BANNER);
	
	fprintf(stderr,"\n\rSystem Memory --------------\n\r");
	// Wait until the Tx queue is empty...
	while(!uartTxEmpty(&logUart));
	//cliCallFunction("rom");
	memRomStatus(stderr);
	fprintf(stderr,"\n\r");
	// Wait until the Tx queue is empty...
	while(!uartTxEmpty(&logUart));
	//cliCallFunction("ram");
	memRamStatus(stderr);
	// Wait until the Tx queue is empty...
	while(!uartTxEmpty(&logUart));
}


