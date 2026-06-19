/**
 * @file mem.h
 * @brief Memory driver — RAM/ROM usage reporting and stack watermarking.
 *
 * Created: 4/24/2021 12:04:39 AM
 *  Author: admin
 */ 


#ifndef MEM_H_
#define MEM_H_

/** @addtogroup mem_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>

// Externals ------------------------------------------------------------------
extern uint16_t __data_start,__data_end,__heap_start, *__brkval;
extern uint16_t _etext,__start_text_window,__stop_text_window,__stop_rodata;

// Inline Functions -----------------------------------------------------------
/**
 * @brief Get the size of program ROM not mapped into data space.
 *
 * @return Program ROM size in bytes.
 */
static inline uint32_t memProgramRomSize()
{
	return(PROGMEM_SIZE-MAPPED_PROGMEM_SIZE);
}

/**
 * @brief Get the size of mapped constant ROM.
 *
 * @return Mapped constant ROM size in bytes.
 */
static inline uint16_t memConstRomSize()
{
	return(MAPPED_PROGMEM_SIZE);
}

/**
 * @brief Get the size of the text segment.
 *
 * @return Text segment size in bytes.
 */
static inline uint16_t memTextSize()
{
	return((uint16_t)&_etext);
}

/**
 * @brief Get the size of mapped constants in flash.
 *
 * @return Constant section size in bytes.
 */
static inline uint16_t memConstSize()
{
	return((uint16_t)&__stop_text_window - (uint16_t)&__start_text_window);
}

/**
 * @brief Get the size of the read-only data section.
 *
 * @return Read-only data size in bytes.
 */
static inline uint16_t memRodataSize()
{
	return((uint16_t)&__stop_rodata - (uint16_t)&__start_text_window);
}

/**
 * @brief Get the size of the OS table section.
 *
 * @return OS table size in bytes.
 */
static inline uint16_t memOsTableSize()
{
	return((uint16_t)&__stop_text_window - (uint16_t)&__stop_rodata);
}

/**
 * @brief Get the size of the initialized data section in RAM.
 *
 * @return Data section size in bytes.
 */
static inline uint16_t memDataSize()
{
	return((uint16_t)&__heap_start - (uint16_t)&__data_start);
}

/**
 * @brief Get the current heap usage.
 *
 * @return Heap size in bytes.
 */
static inline uint16_t memHeapSize()
{
	return((uint16_t)__brkval == 0 ? 0 : (uint16_t)__brkval - (uint16_t) &__heap_start);
}

/**
 * @brief Get the current stack usage.
 *
 * @return Stack size in bytes.
 */
static inline uint16_t memStackSize()
{
	uint16_t stackTop = (uint16_t)&stackTop;
	return(RAMEND - stackTop);
}

/**
 * @brief Get the currently available free RAM between heap and stack.
 *
 * @return Free RAM size in bytes.
 */
static inline uint16_t memFreeSize()
{
	uint16_t stackTop = (uint16_t)&stackTop;
	return((uint16_t)stackTop - ((uint16_t)__brkval == 0 ? (uint16_t) &__heap_start : (uint16_t) __brkval));
}

/**
 * @brief Get total RAM size.
 *
 * @return Total RAM size in bytes.
 */
static inline uint16_t memRamSize()
{
	return(RAMSIZE);
}

// External Functions ---------------------------------------------------------
/**
 * @brief Fill stack memory with a known pattern for usage analysis.
 */
void memStackFill();

/**
 * @brief Get the maximum observed stack usage.
 *
 * @return Maximum stack usage in bytes.
 */
uint16_t memStackSizeMax();

/**
 * @brief Write ROM usage status to a stream.
 *
 * @param file Output stream that receives ROM status information.
 */
void memRomStatus(FILE *file);

/**
 * @brief Write RAM usage status to a stream.
 *
 * @param file Output stream that receives RAM status information.
 */
void memRamStatus(FILE *file);

/** @} */ // end of mem_driver

#endif /* MEM_H_ */