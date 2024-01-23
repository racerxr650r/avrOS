/*
 * lst.h
 *
 * Header file for macros and function declarations to implement generic
 * functions that manage singly linked lists
 * 
 * Created: 1/21/2024
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

#ifndef LST_H_
#define LST_H_

#include "../avrOS.h"

// Macros ---------------------------------------------------------------------
#define lstAppend(head, member) \
        lstOffsetAppend(head, member, offsetof(typeof(member),next));

#define lstRemove(head, member) \
        lstOffsetRemove(head, member, offsetof(typeof(member),next));
#define lstFind(head, member) \
        lstOffsetFind(head, member, offsetof(typeof(member),next));
#define lstInsert(head, member) \
        lstOffsetInsert(head, member, offsetof(typeof(member),next), offsetof(typeof(member),priority));

// Function Declarations ------------------------------------------------------
int lstOffsetAppend(void **head, void *member, size_t nextOffset);
int lstOffsetPush(void **head, void *member, size_t nextOffset);
int lstOffsetRemove(void **head, void *member, size_t nextOffset);
void* lstOffsetPop(void **head, size_t nextOffset);
int lstOffsetInsert(void **head, void *member, size_t nextOffset, size_t priorityOffset);
bool lstOffsetFind(void *head, void *member, size_t nextOffset);
#endif // LST_H_
