/*
 * list.h
 *
 * Data types, macros, and function declarations to implement linked lists
 *
 * Created: 3/27/2026
 * Author : john anderson
 *
 * Copyright (C) 2023 by John Anderson <racerxr650r@gmail.com>
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
#ifndef LIST_H_
#define LIST_H_

#include "../avrOS.h"

// Types ----------------------------------------------------------------------
/*typedef struct LIST_NODE
{
    void *data;
    struct LIST_NODE *next;
} listNode_t;*/

typedef struct LIST
{
    listNode_t *head;
    listNode_t *tail;
    uint32_t size;
} list_t;

// Macros ----------------------------------------------------------------------
#define LIST_INIT() { .head = NULL, .tail = NULL, .size = 0 }

// External Functions ----------------------------------------------------------
/*void listInit(list_t *list);
int listAdd(list_t *list, void *data);
int listRemove(list_t *list, void *data);
void* listGet(list_t *list, uint32_t index);
uint32_t listSize(list_t *list);*/

#endif  // LIST_H_
