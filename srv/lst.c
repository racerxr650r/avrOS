/*
 * lst.c
 *
 * Implements generic singly linked list management functions
 * 
 * Created: 4/25/2021 6:01:41 PM
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

int lstOffsetAppend(void **head, void *member, size_t nextOffset)
{
	int ret = 0;
	
	// If sm is valid...
	if(member != NULL)
	{
		// If there is no elements in the current list...
		if(*head == NULL)
		{
			*head = member;
			*(void **)((size_t)member+nextOffset) = NULL;
		}
		// Else there is at least one element in the current list...
		else
		{
			void *curr = *head, *prev = NULL;

			// While this is not the end of the list...
			while(curr!=NULL)
			{
				prev = curr;
				curr = *(void **)((size_t)curr+nextOffset);
			}
			*(void **)((size_t)prev+nextOffset) = member;
			*(void **)((size_t)member+nextOffset) = NULL;
		}
	}
	else
		ret = -1;

	return(ret);
}

int lstOffsetPush(void **head, void *member, size_t nextOffset)
{
	int ret = 0;

	return(ret);
}

int lstOffsetRemove(void **head, void *member, size_t nextOffset)
{
	int ret = 0;

	return(ret);
}

void* lstOffsetPop(void **head, size_t nextOffset)
{
	int *member = NULL;

	return(member);
}

int lstOffsetInsert(void **head, void *member, size_t nextOffset, size_t priorityOffset)
{
	int ret = 0;

	return(ret);
}

bool lstOffsetFind(void *head, void *member, size_t nextOffset)
{
	bool ret = false;

	return(ret);
}