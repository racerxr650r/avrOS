/*
 * mem.c
 *
 * Implements the memory status functions for the CLI
 *
 * Created: 4/3/2021 5:28:23 PM
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

// Internal function prototypes ----------------------------------------------

// CLI Commands ---------------------------------------------------------------
#ifdef MEM_CLI
ADD_COMMAND("ram",ramCmd,true);
#endif
int ramCmd(int argc, char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);
	
	memRamStatus(stdout);
	
	return(0);
}

#ifdef MEM_CLI
ADD_COMMAND("rom",romCmd,true);
#endif
int romCmd(int argc, char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);
	
	memRomStatus(stdout);
	
	return(0);
}

// Internal Functions --------------------------------------------------------

// External Functions ---------------------------------------------------------
static uint8_t fillPattern[] = {0xde,0xad,0xbe,0xef};

// Return the whole portion of the percentage representing the ratio provided
int percentWhole(uint32_t den, uint32_t div)
{
	return((int)(den*100/div));
}

// Return the digits right of the decimal point of the percentage representing
// the ratio provided (Who needs bloated floating point libs?)
int percentPlaces(uint32_t den, uint32_t div)
{
	return((int)((den*100%div)*100/div));
}

// Fill the stack area of RAM with a pattern so we can detect a max size for the stack
void memStackFill()
{
	// If the heap is empty, use the heap start pointer, else use the end of heap counter maintained by malloc
	uint16_t heapTop = (uint16_t)__brkval == 0 ? (uint16_t) &__heap_start : (uint16_t)__brkval;
	// Mod 4 the address to locate the offset in the fill pattern
	uint16_t  offset= heapTop&0x0003;
	
	// Do this to guaranty the stackTop points to the top of the stack
	do
	{
		// Locate the "top" of the stack (it actually grows down from the top of RAM)
		uint16_t stackTop = (uint16_t)&stackTop;
	
		while(heapTop!=stackTop)
		{
			((uint8_t *)heapTop)[0] = fillPattern[offset];
			++heapTop;
			if(++offset>3)
				offset = 0;
		}
	}while(0);
}

// From the current stack pointer, scan memory until you find the fill pattern
// This will determine the maximum stack size to this point
uint16_t memStackSizeMax()
{
	uint16_t memAddress = RAMEND;
	uint8_t  memMatch = 0;
	// If the heap is empty, use the heap start pointer, else use the end of heap counter maintained by malloc
	uint16_t heapTop = (uint16_t)__brkval == 0 ? (uint16_t) &__heap_start : (uint16_t)__brkval;
	// Mod 8 the address to locate the offset in the fill pattern
	uint16_t  offset = memAddress&0x0003;
	
	// Scan beyond the top of the current stack point until you find 8 matching bytes of the fill pattern
	while(memMatch<4 && memAddress>heapTop)
	{
		if(((uint8_t *)memAddress)[0] == fillPattern[offset])
			++memMatch;
		else
			memMatch = 0;
		--memAddress;
		if(--offset>3)
			offset = 3;
	}
	
	memAddress += 4;
	
	return(RAMEND - memAddress);
}

void memRomStatus(FILE *file)
{
	uint32_t memProgRom = memProgramRomSize();
	uint16_t memConstRom = memConstRomSize();
	uint16_t memText = memTextSize();
	uint32_t memProgRomFree = memProgRom-memText;
	uint16_t memRodata = memRodataSize();
	uint16_t memOsTable = memOsTableSize();
	uint16_t memConstRomFree = memConstRom - (memRodata+memOsTable);

	fprintf(file,FG_CYAN "\n\rProgram ROM:" RESET " %6lu\n\r",memProgRom);
	fprintf(file,FG_CYAN "       text:" RESET " %6u (%2d.%02d%%)\n\r",memText,percentWhole(memText,memProgRom),percentPlaces(memText,memProgRom));
	fprintf(file,FG_CYAN "       free:" RESET " %6lu (%2d.%02d%%)\n\n\r",memProgRomFree,percentWhole(memProgRomFree,memProgRom),percentPlaces(memProgRomFree,memProgRom));
	
	fprintf(file,FG_CYAN "  Const ROM:" RESET " %6u\n\r",memConstRom);
	fprintf(file,FG_CYAN "     rodata:" RESET " %6u (%2d.%02d%%)\n\r",memRodata,percentWhole(memRodata,memConstRom),percentPlaces(memRodata,memConstRom));
	fprintf(file,FG_CYAN "  os tables:" RESET " %6u (%2d.%02d%%)\n\r",memOsTable,percentWhole(memOsTable,memConstRom),percentPlaces(memOsTable,memConstRom));
	fprintf(file,FG_CYAN "       free:" RESET " %6u (%2d.%02d%%)\n\r",memConstRomFree,percentWhole(memConstRomFree,memConstRom),percentPlaces(memConstRomFree,memConstRom));
}

void memRamStatus(FILE *file)
{
	uint16_t memRam = memRamSize();
	uint16_t memData = memDataSize();
	uint16_t memHeap = memHeapSize();
	uint16_t memStack = memStackSize();
	uint16_t memStackMax = memStackSizeMax();
	uint16_t memFree = memFreeSize();

	fprintf(file,FG_CYAN "\n\r        RAM:" RESET " %6u\n\r",memRam);
	fprintf(file,FG_CYAN "       data:" RESET " %6u (%2d.%02d%%)\n\r",memData,percentWhole(memData,memRam),percentPlaces(memData,memRam));
	fprintf(file,FG_CYAN "       heap:" RESET " %6u (%2d.%02d%%)\n\r",memHeap,percentWhole(memHeap,memRam),percentPlaces(memHeap,memRam));
	fprintf(file,FG_CYAN " curr stack:" RESET " %6u (%2d.%02d%%)\n\r",memStack,percentWhole(memStack,memRam),percentPlaces(memStack,memRam));
#ifdef MEM_STATS	
	fprintf(file,FG_CYAN "  max stack:" RESET " %6u (%2d.%02d%%)\n\r",memStackMax,percentWhole(memStackMax,memRam),percentPlaces(memStackMax,memRam));
#endif	
	fprintf(file,FG_CYAN "       free:" RESET " %6u (%2d.%02d%%)\n\r",memFree,percentWhole(memFree,memRam),percentPlaces(memFree,memRam));
}