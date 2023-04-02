/*
 * avrOS.h
 *
 * Includes, macros, and definitions required by OS and application source
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

#ifndef AVROS_H_
#define AVROS_H_

// Toolchain/Library header files ****************************************************
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>

// Constants ******************************************************************
#define AVRSMOS_OS

// General definitions ********************************************************
#define OK		0
#define ERR		1

#define DISABLE 0
#define ENABLE	1

// avrOS System Header Files ************************************************
#include "avrOSConfig.h"
#include "srv/queue.h"
#include "drv/cpu.h"
#include "drv/sys.h"
#include "drv/uart.h"
#include "drv/dac.h"
#include "srv/fsm.h"
#include "srv/cli.h"
#include "srv/tmr.h"
#include "srv/log.h"
#include "srv/mem.h"
//#include "crtDrv.h"
//#include "delaySrv.h"
//#include "spiDrv.h"
//#include "sndDrv.h"
//#include "uart.h"
//#include "ps2Drv.h"
//#include "winSrv.h"
//#include "uiMgr.h"
//#include "tckObj.h"
//#include "txtObj.h"

// Macros *********************************************************************
#define UNUSED(x) (void)(x)

#define CONCAT_( x,y ) x##y
#define CONCAT( x,y ) CONCAT_( x,y )

#define DEFAULT_OR_ARG(z,a,val,...)		val

#define CONCAT_THREE(a,b,c)				a ## b ## c
#define UNIQUENAME(prefix, func, num)	CONCAT_THREE( prefix , func, num )
#define UNIQUEIDENT(prefix)				UNIQUENAME( prefix , __FUNCTION__ , __LINE__ )

#define SECTION(sectionName)			__attribute__((__used__,__section__(#sectionName)))

//#define ROM_STR(prefix,str)			static char const UNIQUENAME(prefix) PROGMEM = {str}
#define ROM_STR(var_name,str)			static char const var_name[] PROGMEM = {str}
#define ROM_STR_G(var_name,str)			char const var_name[] PROGMEM = {str}


// External globals ***********************************************************
//extern const char newLine;
//extern char const winContextStr[], srlContextStr[];

// External functions *********************************************************
//extern uint16_t freeRam(void);

#endif /* AVROS_H_ */
