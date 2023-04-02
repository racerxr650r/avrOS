/*
 * cli.h
 *
 * Header for CLI manager and API. Includes macros to create new command tokens
 * with associated function handlers
 *
 * Created: 2/28/2021 4:14:29 PM
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
#ifndef __CLI_H
#define __CLI_H

// Includes -------------------------------------------------------------------
#include "../avrOS.h"

// Types ----------------------------------------------------------------------
typedef int (*commandHandler_t)(int argc, char *argv[]);

typedef struct
{
    const char			*commandStr;
    commandHandler_t	funcPtr;
	bool				repeatable;
} cliCommand_t;

// This macro adds a command string and a function to the cli table
#define ADD_COMMAND(name,function,...)	static int function(int argc, char *argv[]); \
										const static cliCommand_t SECTION(CLI_CMDS) CONCAT(function,__COUNTER__) = { .commandStr = name, .funcPtr = &function, .repeatable = DEFAULT_OR_ARG(,##__VA_ARGS__,__VA_ARGS__,false)};


// Constants ------------------------------------------------------------------
#define KEYCODE_UP          0x11
#define KEYCODE_DOWN        0x12
#define KEYCODE_LEFT        0x13
#define KEYCODE_RIGHT       0x14

// External Functions----------------------------------------------------------
extern void cliInit();
extern int cliCallFunction(char *commandLine);
extern int cliReceiveFunc(fsmStateMachine_t *state);

#endif /* __CLI_H */

