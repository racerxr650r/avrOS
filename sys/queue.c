/*
 * queue.c
 *
 * Functions to implement circular queue data structure.  
 *
 * Created: 2/19/2021 2:33:18 PM
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
#include "../avrOS.h"

// Externs --------------------------------------------------------------------
extern void *__start_QUE_TABLE,*__stop_QUE_TABLE;

// Command line interface -----------------------------------------------------
#ifdef QUE_CLI
ADD_COMMAND("que",queCmd,true);
#endif

static int queCmd(int argc, char *argv[])
{
	// Walk the table of queues
	queDescriptor_t *descr = (queDescriptor_t *)&__start_QUE_TABLE;
	for(; descr < (queDescriptor_t *)&__stop_QUE_TABLE; ++descr)
	{
		if(argc<2 || (argc==2 && !strcmp(descr->name,argv[1])))
		{
			printf(BOLD UNDERLINE FG_BLUE "%-20s Capacity: %d\n\r" RESET,descr->name,descr->capacity);
			printf("\tIn:%8lu\tOut:%8lu\tMax:%8lu\n\r",descr->stats->in,descr->stats->out,descr->stats->max);
			printf("\tOverflow:%8lu\tUnderflow:%8lu\n\r",descr->stats->overflow,descr->stats->underflow);
		}
	}
	return(0);
}

// External functions ************************************************
uint16_t queGetSize(volatile queue_t *que)
{
	const queDescriptor_t *descr = que->descr;
	uint16_t			ret;
	
	if(queIsEmpty(que))
		ret = 0;
	else if(queIsFull(que))
		ret = descr->capacity;
	else if(que->tail==que->head)
		ret = descr->capacity-1;
	else if(que->tail>=que->head)
		ret = que->tail-que->head;
	else
		ret = descr->capacity-(que->head-que->tail);
		
	return(ret);	
}

bool queGet(volatile queue_t *que, void *element)
{
	const queDescriptor_t *descr = que->descr;
	bool			ret = false;
	
	// Start of critical section
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// If not empty
		if(que->head < descr->capacity)
		{
			// Get the element from the head of the queue
			memcpy(element,&(descr->buffer[que->head*descr->sizeOfElement]),descr->sizeOfElement);
			// *ch = que->buffer[queue->head];
#ifdef QUE_STATS
			++descr->stats->out;
#endif
			// If the buffer was previously full...
			if(que->tail == descr->capacity)
			{
				// Point the tail at the new empty slot
				que->tail = que->head;
				evntTrigger(descr->event,QUE_EVENT_NOT_FULL);
			}
			
			// Increment the head pointer and wrap if needed
			if(++que->head == descr->capacity)
				que->head = 0;

			// If the buffer is now empty...
			if(que->tail == que->head)
			{
				// Set the head pointer to show that
				que->head = descr->capacity;
				evntTrigger(descr->event,QUE_EVENT_EMPTY);
			}

			ret = true;
		}
#ifdef QUE_STATS
		else
			++descr->stats->underflow;
#endif
		
	} // End of critical section
	return(ret);
}

bool quePut(volatile queue_t *que, void *element)
{
	const queDescriptor_t *descr = que->descr;
	bool			ret = false;
	
	// Start of critical section
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// If not already full...
		if(que->tail < descr->capacity)
		{
			// If not full now...
			if(que->tail != que->head)
			{
				// Put the element into the buffer
				memcpy(&(descr->buffer[que->tail*descr->sizeOfElement]),element,descr->sizeOfElement);
				//que->buffer[queue->tail] = ch;
#ifdef QUE_STATS
				++descr->stats->in;
#endif			
				// If the buffer was previously empty...
				if(que->head == descr->capacity)
				{
					// Point the head to the char just added
					que->head = que->tail;
					evntTrigger(descr->event, QUE_EVENT_NOT_EMPTY);
				}
				// Increment the tail pointer and wrap
				if(++que->tail==descr->capacity)
					que->tail = 0;
			
				// Is the queue now full...	
				if(que->tail == que->head)
				{
					// Point the tail beyond the buffer
					que->tail = descr->capacity;
					evntTrigger(descr->event, QUE_EVENT_FULL);
				}
				
#ifdef QUE_STATS
				// If the current capacity of queueue exceeds the previous max, update the max
				uint16_t	size = queGetSize(que);
				if(size>descr->stats->max)
					descr->stats->max = size;
#endif			
				
				ret = true;
			}
		}
#ifdef QUE_STATS
		else
			++descr->stats->overflow;
#endif
	} // End of critical section
	return(ret);
}
