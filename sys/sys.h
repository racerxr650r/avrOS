/*
 * sys.h
 *
 * Header for system functions, including system initialize and counter/tick
 *
 * Created: 4/17/2021 9:12:55 PM
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

#ifndef SYS_H_
#define SYS_H_

// Constants ------------------------------------------------------------------
#define SYS_TIMER_TCB0  0x0B00
#define SYS_TIMER_TCB1  0x0B10
#define SYS_TIMER_TCB2  0x0B20

#define EVENT_TYPE_TICK  EVENT_TYPE_1

// External Functions ---------------------------------------------------------
bool sysInit();
void sysSetTickFreq(uint16_t sysTickFreq);
uint16_t sysGetTickFreq();
uint32_t sysGetTickCount();
void sysSleep();

#endif /* SYS_H_ */
