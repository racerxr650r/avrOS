/**
 * @file log.h
 * @brief Logging service — severity-level macros and log instance registration.
 *
 * Created: 4/25/2021 6:12:14 PM
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

#ifndef LOG_H_
#define LOG_H_

/** @addtogroup log_service
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdio.h>

typedef struct
{
	char *name;
	FILE *outFile;
} logInstance_t;

/**
 * @brief Add a new log instance and register its initializer.
 *
 * This macro declares a static FILE object, creates a named log instance,
 * and registers `logInit` as an initializer for that instance.
 *
 * @param logName Name of the log instance symbol.
 * @param logFile Name of the static FILE object backing the log output.
 */
#define ADD_LOG(logName, logFile) \
				static FILE logFile; \
				const static logInstance_t logName = { .name = #logName, .outFile = &logFile }; \
				ADD_INITIALIZER(logName, logInit, (void *)&logName);

// LOG message macros ---------------------------------------------------------
#if LOG_FORMAT == 1
/**
 * @brief Log an informational message.
 *
 * Emits a formatted INFO message to `stderr` using the active log format.
 *
 * @param fmt_str `printf`-style format string.
 * @param ... Optional format arguments.
 */
#define INFO(fmt_str,...)	do{\
	fprintf(stderr,FG_GREEN BOLD "\n\r%s: " RESET,"INFO"); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
/**
 * @brief Log a warning message.
 *
 * Emits a formatted WARN message to `stderr` using the active log format.
 *
 * @param fmt_str `printf`-style format string.
 * @param ... Optional format arguments.
 */
#define WARN(fmt_str,...)	do{\
	fprintf(stderr,FG_ORANGE BOLD "\n\r%s: " RESET,"WARN"); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
/**
 * @brief Log an error message.
 *
 * Emits a formatted ERROR message to `stderr` using the active log format.
 *
 * @param fmt_str `printf`-style format string.
 * @param ... Optional format arguments.
 */
#define ERROR(fmt_str,...)	do{\
	fprintf(stderr,FG_RED BOLD "\n\r%s: " RESET,"ERR "); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
/**
 * @brief Log a critical message and halt execution.
 *
 * Emits a formatted CRITICAL message, prints a stop banner, and enters an
 * infinite loop.
 *
 * @param fmt_str `printf`-style format string.
 * @param ... Optional format arguments.
 */
#define CRITICAL(fmt_str,...)	do{\
	fprintf(stderr,FG_WHITE BG_RED BOLD "\n\r%s: " RESET,sysGetTickCount(),"CRIT"); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
	fprintf(stderr,FG_WHITE BG_RED BOLD BLINKING "\n\r+++ System Stopped +++" RESET); \
	while(1);\
}while(0)
#elif LOG_FORMAT == 2
#define INFO(fmt_str,...)	do{\
	fprintf(stderr,"\n\r%lu:%s: ",sysGetTickCount(),"INFO"); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define WARN(fmt_str,...)	do{\
	fprintf(stderr,"\n\r%lu:%s: ",sysGetTickCount(),"WARN"); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define ERROR(fmt_str,...)	do{\
	fprintf(stderr,"\n\r%lu:%s: ",sysGetTickCount(),"ERR "); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define CRITICAL(fmt_str,...)	do{\
	fprintf(stderr,"\n\r%lu:%s: ",sysGetTickCount(),"CRIT"); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
	fprintf(stderr,"\n\r+++ System Stopped +++"); \
	while(1);\
}while(0)
#elif LOG_FORMAT == 3
#define INFO(fmt_str,...)	do{\
	fprintf(stderr,FG_GREEN BOLD "\n\r%lu:%s:%s:%s: " RESET,sysGetTickCount(),"INFO",fsmGetCurrentStateMachineName(),fsmGetCurrentStateName(fsmGetCurrentStateMachine())); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define WARN(fmt_str,...)	do{\
	fprintf(stderr,FG_ORANGE BOLD "\n\r%lu:%s:%s:%s: " RESET,sysGetTickCount(),"WARN",fsmGetCurrentStateMachineName(),fsmGetCurrentStateName(fsmGetCurrentStateMachine())); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define ERROR(fmt_str,...)	do{\
	fprintf(stderr,FG_RED BOLD "\n\r%lu:%s:%s:%s: " RESET,sysGetTickCount(),"ERR ",fsmGetCurrentStateMachineName(),fsmGetCurrentStateName(fsmGetCurrentStateMachine())); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define CRITICAL(fmt_str,...)	do{\
	fprintf(stderr,FG_WHITE BG_RED BOLD "\n\r%lu:%s:%s:%s: " RESET,sysGetTickCount(),"CRIT",fsmGetCurrentStateMachineName(),fsmGetCurrentStateName(fsmGetCurrentStateMachine())); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
	fprintf(stderr,FG_WHITE BG_RED BOLD BLINKING "\n\r+++ System Stopped +++" RESET); \
	while(1);\
}while(0)
#elif LOG_FORMAT == 4
#define INFO(fmt_str,...)	do{\
	fprintf(stderr,"\n\r%lu:%s:%s:%d: ",sysGetTickCount(),"INFO",__FUNCTION__,__LINE__); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define WARN(fmt_str,...)	do{\
	fprintf(stderr,"\n\r%lu:%s:%s:%d: ",sysGetTickCount(),"WARN",__FUNCTION__,__LINE__); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define ERROR(fmt_str,...)	do{\
	fprintf(stderr,"\n\r%lu:%s:%s:%d: ",sysGetTickCount(),"ERR ",__FUNCTION__,__LINE__); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define CRITICAL(fmt_str,...)	do{\
	fprintf(stderr,"\n\r%lu:%s:%s:%d: ",sysGetTickCount(),"CRIT",__FUNCTION__,__LINE__); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
	fprintf(stderr,"\n\r+++ System Stopped +++"); \
	while(1);\
}while(0)
#endif

// According to LOG_LEVEL specified, set unwanted messages to null
#if LOG_LEVEL <= 3
#undef INFO
#define INFO(...)
#endif
#if LOG_LEVEL <= 2
#undef WARN
#define WARN(...)
#endif
#if LOG_LEVEL <= 1
#undef ERROR
#define ERROR(...)
#endif
#if LOG_LEVEL <=1
#undef CRITICAL
#define CRITICAL(...)
#endif

// External Functions ---------------------------------------------------------
/**
 * @brief Log current RAM usage statistics.
 */
void logRam();

/**
 * @brief Log current ROM usage statistics.
 */
void logRom();

/**
 * @brief Write a newline to the active log output stream.
 */
void logNewLine();

/** @} */ // end of log_service

#endif /* LOG_H_ */
