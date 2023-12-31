/*
 * log.h
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

typedef struct
{
	char *name;
	FILE *outFile;
} logInstance_t;

// Add a log instance macro
#define ADD_LOG(logName, logFile) \
                const static logInstance_t logName = { .name = #logName, .outFile = logFile }; \
                ADD_STATE_MACHINE(logName ## _SM,logInit,FSM_SYS | 0x01, (void *)&logName);
        

// LOG message macros ---------------------------------------------------------
#if LOG_FORMAT == 0
#define INFO(...)
#define WARN(...)
#define ERROR(...)
#define CRITICAL(...)
#elif LOG_FORMAT == 1
#define INFO(fmt_str,...)	do{\
	fprintf(stderr,FG_GREEN BOLD "\n\r%s: " RESET,"INFO"); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define WARN(fmt_str,...)	do{\
	fprintf(stderr,FG_ORANGE BOLD "\n\r%s: " RESET,"WARN"); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define ERROR(fmt_str,...)	do{\
	fprintf(stderr,FG_RED BOLD "\n\r%s: " RESET,"ERR "); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define CRITICAL(fmt_str,...)	do{\
	fprintf(stderr,FG_WHITE BG_RED BOLD "\n\r%s: " RESET,sysGetTick(),"CRIT"); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
	fprintf(stderr,FG_WHITE BG_RED BOLD BLINKING "\n\r+++ System Stopped +++" RESET); \
	while(1);\
}while(0)
#elif LOG_FORMAT == 2
#define INFO(fmt_str,...)	do{\
	fprintf(stderr,"\n\r%lu:%s: ",sysGetTick(),"INFO"); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define WARN(fmt_str,...)	do{\
	fprintf(stderr,"\n\r%lu:%s: ",sysGetTick(),"WARN"); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define ERROR(fmt_str,...)	do{\
	fprintf(stderr,"\n\r%lu:%s: ",sysGetTick(),"ERR "); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define CRITICAL(fmt_str,...)	do{\
	fprintf(stderr,"\n\r%lu:%s: ",sysGetTick(),"CRIT"); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
	fprintf(stderr,"\n\r+++ System Stopped +++"); \
	while(1);\
}while(0)
#elif LOG_FORMAT == 3
#define INFO(fmt_str,...)	do{\
	fprintf(stderr,FG_GREEN BOLD "\n\r%lu:%s:%s:%s: " RESET,sysGetTick(),"INFO",fsmGetCurrentStateMachineName(),fsmGetCurrentStateName(fsmGetCurrentStateMachine())); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define WARN(fmt_str,...)	do{\
	fprintf(stderr,FG_ORANGE BOLD "\n\r%lu:%s:%s:%s: " RESET,sysGetTick(),"WARN",fsmGetCurrentStateMachineName(),fsmGetCurrentStateName(fsmGetCurrentStateMachine())); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define ERROR(fmt_str,...)	do{\
	fprintf(stderr,FG_RED BOLD "\n\r%lu:%s:%s:%s: " RESET,sysGetTick(),"ERR ",fsmGetCurrentStateMachineName(),fsmGetCurrentStateName(fsmGetCurrentStateMachine())); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define CRITICAL(fmt_str,...)	do{\
	fprintf(stderr,FG_WHITE BG_RED BOLD "\n\r%lu:%s:%s:%s: " RESET,sysGetTick(),"CRIT",fsmGetCurrentStateMachineName(),fsmGetCurrentStateName(fsmGetCurrentStateMachine())); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
	fprintf(stderr,FG_WHITE BG_RED BOLD BLINKING "\n\r+++ System Stopped +++" RESET); \
	while(1);\
}while(0)
#elif LOG_FORMAT == 4
#define INFO(fmt_str,...)	do{\
	fprintf(stderr,"\n\r%lu:%s:%s:%d: ",sysGetTick(),"INFO",__FUNCTION__,__LINE__); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define WARN(fmt_str,...)	do{\
	fprintf(stderr,"\n\r%lu:%s:%s:%d: ",sysGetTick(),"WARN",__FUNCTION__,__LINE__); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define ERROR(fmt_str,...)	do{\
	fprintf(stderr,"\n\r%lu:%s:%s:%d: ",sysGetTick(),"ERR ",__FUNCTION__,__LINE__); \
	fprintf(stderr,fmt_str, ##__VA_ARGS__); \
}while(0)
#define CRITICAL(fmt_str,...)	do{\
	fprintf(stderr,"\n\r%lu:%s:%s:%d: ",sysGetTick(),"CRIT",__FUNCTION__,__LINE__); \
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

#endif /* LOG_H_ */
