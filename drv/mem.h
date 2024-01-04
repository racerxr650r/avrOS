/*
 * mem.h
 *
 * Created: 4/24/2021 12:04:39 AM
 *  Author: admin
 */ 


#ifndef MEM_H_
#define MEM_H_

// Externals ------------------------------------------------------------------
extern uint16_t __data_start,__data_end,__heap_start, *__brkval;
extern uint16_t _etext,__start_text_window,__stop_text_window,__stop_rodata;

// Inline Functions -----------------------------------------------------------
static inline uint32_t memProgramRomSize()
{
	return(PROGMEM_SIZE-MAPPED_PROGMEM_SIZE);
}

static inline uint16_t memConstRomSize()
{
	return(MAPPED_PROGMEM_SIZE);
}

static inline uint16_t memTextSize()
{
	return((uint16_t)&_etext);
}

static inline uint16_t memConstSize()
{
	return((uint16_t)&__stop_text_window - (uint16_t)&__start_text_window);
}

static inline uint16_t memRodataSize()
{
	return((uint16_t)&__stop_rodata - (uint16_t)&__start_text_window);
}

static inline uint16_t memOsTableSize()
{
	return((uint16_t)&__stop_text_window - (uint16_t)&__stop_rodata);
}

static inline uint16_t memDataSize()
{
	return((uint16_t)&__heap_start - (uint16_t)&__data_start);
}

static inline uint16_t memHeapSize()
{
	return((uint16_t)__brkval == 0 ? 0 : (uint16_t)__brkval - (uint16_t) &__heap_start);
}

static inline uint16_t memStackSize()
{
	uint16_t stackTop = (uint16_t)&stackTop;
	return(RAMEND - stackTop);
}

static inline uint16_t memFreeSize()
{
	uint16_t stackTop = (uint16_t)&stackTop;
	return((uint16_t)stackTop - ((uint16_t)__brkval == 0 ? (uint16_t) &__heap_start : (uint16_t) __brkval));
}

static inline uint16_t memRamSize()
{
	return(RAMSIZE);
}

// External Functions ---------------------------------------------------------
void memStackFill();
uint16_t memStackSizeMax();
void memRomStatus(FILE *file);
void memRamStatus(FILE *file);

#endif /* MEM_H_ */