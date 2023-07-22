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
	Queue_t *descr = (Queue_t *)&__start_QUE_TABLE;
	for(; descr < (Queue_t *)&__stop_QUE_TABLE; ++descr)
	{
		if(argc<2 || (argc==2 && !strcmp(descr->name,argv[1])))
		{
			printf("Queue: %s\n\r",descr->name);
			printf("\tIn:%8lu\tOut:%8lu\tMax:%8lu\n\r",descr->stats->in,descr->stats->out,descr->stats->max);
			printf("\tOverflow:%8lu\tUnderflow:%8lu\n\r",descr->stats->overflow,descr->stats->underflow);
		}
	}
	return(0);
}

// External functions ************************************************
bool queEmpty(const Queue_t *que)
{
	return(que->queue->head == que->queue->size?true:false);
}

bool queFull(const Queue_t *que)
{
	return(que->queue->tail == que->queue->size?true:false);
}

uint8_t queSize(const Queue_t *que)
{
	return(que->queue->size);
}

#ifdef QUE_STATS
uint8_t queMax(const Queue_t *que)
{
	return(que->stats->max);
}
#endif

uint8_t queCount(const Queue_t *que)
{
	volatile QueueState_t	*queue = que->queue;
	uint8_t			ret;
	
	if(queEmpty(que))
		ret = 0;
	else if(queFull(que))
		ret = queue->size;
	else if(queue->tail==queue->head)
		ret = queue->size-1;
	else if(queue->tail>=queue->head)
		ret = queue->tail-queue->head;
	else
		ret = queue->size-(queue->head-queue->tail);
		
	return(ret);	
}

bool queGet(const Queue_t *que, char *ch)
{
	volatile QueueState_t	*queue = que->queue;	
	bool			ret = false;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// If not empty
		if(queue->head < queue->size)
		{
			// Get the char from the head of the queue
			*ch = que->buffer[queue->head];
#ifdef QUE_STATS
			++que->stats->out;
#endif
			// If the buffer was previously full...
			if(queue->tail == queue->size)
			{
				// Point the tail at the new empty slot
				queue->tail = queue->head;
				evntTrigger(que->event,QUE_EVENT_NOT_FULL);
			}
			
			// Increment the head pointer and wrap if needed
			if(++queue->head == queue->size)
				queue->head = 0;

			// If the buffer is now empty...
			if(queue->tail == queue->head)
			{
				// Set the head pointer to show that
				queue->head = queue->size;
				evntTrigger(que->event,QUE_EVENT_EMPTY);
			}

			ret = true;
		}
#ifdef QUE_STATS
		else
			++que->stats->underflow;
#endif
		
	}
	return(ret);
}

bool quePut(const Queue_t *que, char ch)
{
	volatile QueueState_t	*queue = que->queue;
	bool			ret = false;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// If not already full...
		if(queue->tail < queue->size)
		{
			// If not full now...
			if(queue->tail != queue->head)
			{
				// Put the char into the buffer
				que->buffer[queue->tail] = ch;
#ifdef QUE_STATS
				++que->stats->in;
#endif			
				// If the buffer was previously empty...
				if(queue->head == queue->size)
				{
					// Point the head to the char just added
					queue->head = queue->tail;
					evntTrigger(que->event, QUE_EVENT_NOT_EMPTY);
				}
				// Increment the tail pointer and wrap
				if(++queue->tail==queue->size)
					queue->tail = 0;
			
				// Is the queue now full...	
				if(queue->tail == queue->head)
				{
					// Point the tail beyond the buffer
					queue->tail = queue->size;
					evntTrigger(que->event, QUE_EVENT_FULL);
				}
				
#ifdef QUE_STATS
				// If the current size of queueue exceeds the previous max, update the max
				uint8_t	size = queSize(que);
				if(size>que->stats->max)
					que->stats->max = size;
#endif			
				
				ret = true;
			}
		}
#ifdef QUE_STATS
		else
			++que->stats->overflow;
#endif
	}
	return(ret);
}
