/**
 * @brief CLI manager and API
 * @details This header file implements the CLI macros and datatypes
 *
 * @date 2/28/2021
 * @author john anderson
 * @copyright Copyright (C) 2021 by John Anderson <racerxr650r@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef __CLI_H
#define __CLI_H

// Includes -------------------------------------------------------------------
#include "../avrOS.h"

// Types ----------------------------------------------------------------------
typedef struct cliCommand_struct cliCommand_t;
typedef int (*commandHandler_t)(int argc, char *argv[]);

typedef struct
{
    char				key;
    char				commandLine[MAX_CMD_LINE];
    char				previousCommand[MAX_CMD_LINE];
    int				    lineCounter;
    char*			    argV[MAX_ARGS];
    int				    argC;
    commandHandler_t	cmdFuncPtr;
    cliCommand_t		*currentCommand;
}cliState_t;

typedef struct
{
    char	  *name;
    FILE      *inFile, *outFile;
    UART_t	  *uart;
    cliState_t state;
//  crtWindow *window;
}cliInstance_t;

struct cliCommand_struct
{
    const char			*commandStr;
    commandHandler_t	funcPtr;
    bool				repeatable;
    cliCommand_t        *rootCommand;
};

// This macro adds a new instance of a CLI
#define ADD_CLI(cliName, cliFile) \
        static FILE cliFile; \
        const static cliInstance_t cliName = { .name = #cliName, .inFile = &cliFile, .outFile = &cliFile }; \
        ADD_STATE_MACHINE(cliName ## _SM,cliInit,FSM_SRV | 0x3f, (void *)&cliName);

// This macro adds a command string and a function to the cli table
#define ADD_COMMAND(name,function,...)	static int function(int argc, char *argv[]); \
                                        const static cliCommand_t SECTION(CLI_CMDS) CONCAT(function,__COUNTER__) = { .commandStr = name, .funcPtr = &function, .repeatable = DEFAULT_OR_ARG(,##__VA_ARGS__,__VA_ARGS__,false), .rootCommand = NULL};

// This macro adds a command string and a function to the cli table
#define ADD_SUBCOMMAND(name,function,root,...)	static int function(int argc, char *argv[]); \
                                                const static cliCommand_t SECTION(CLI_CMDS) CONCAT(function,__COUNTER__) = { .commandStr = name, .funcPtr = &function, .repeatable = DEFAULT_OR_ARG(,##__VA_ARGS__,__VA_ARGS__,false), .rootCommand = &root};

// Constants ------------------------------------------------------------------
#define KEYCODE_UP          0x11
#define KEYCODE_DOWN        0x12
#define KEYCODE_LEFT        0x13
#define KEYCODE_RIGHT       0x14

#define CLEAR_SCREEN        "\e[2J"
#define CURSOR_HOME         "\e[H"
#define CURSOR_HIDE         "\e[?25l"
#define CURSOR_UNHIDE       "\e[?25h"

#define BEL                 '\a'    // Terminal Bell
#define BS                  '\b'    // Backspace
#define HT                  '\t'    // Horizontal Tab
#define LF                  '\n'    // Linefeed
#define VT                  '\v'    // Vertical Tab
#define FF                  '\f'    // Formfeed/New Page
#define CR                  '\r'    // Carriage Return
#define ESC                 '\e'    // Escape Character
#define DEL                 '\x07f'

#define RESET               "\e[0m"
#define BOLD                "\e[1m"
#define NOT_BOLD            "\e[22m"
#define DIM                 "\e[2m"
#define NOT_DIM             "\e[22m"
#define ITALIC              "\e[3m"
#define NOT_ITALIC          "\e[23m"
#define UNDERLINE           "\e[4m"
#define NOT_UNDERLINE       "\e[24m"
#define BLINKING            "\e[5m"
#define NOT_BLINKING        "\e[25m"
#define INVERSE             "\e[7m"
#define NOT_INVERSE         "\e[27m"
#define HIDDEN              "\e[8m"
#define NOT_HIDDEN          "\e[28m"
#define STRIKETHROUGH       "\e[9m"
#define NOT_STRIKETHOUGH    "\e[29m"

#define FG_BLACK            "\e[30m"
#define FG_RED              "\e[31m"
#define FG_GREEN            "\e[32m"
#define FG_ORANGE           "\e[33m"
#define FG_BLUE             "\e[34m"
#define FG_PURPLE           "\e[35m"
#define FG_CYAN             "\e[36m"
#define FG_WHITE            "\e[37m"
#define FG_DEFAULT          "\e[39m"

#define BG_BLACK            "\e[40m"
#define BG_RED              "\e[41m"
#define BG_GREEN            "\e[42m"
#define BG_ORANGE           "\e[43m"
#define BG_BLUE             "\e[44m"
#define BG_PURPLE           "\e[45m"
#define BG_CYAN             "\e[46m"
#define BG_WHITE            "\e[47m"
#define BG_DEFAULT          "\e[49m"

// External Functions----------------------------------------------------------
extern int cliCallFunction(char *commandLine);

#endif /* __CLI_H */

