/*
 * sys.h
 *
 * Header for system functions, including system counter/tick
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
#define FREQ_1_MHZ	1000000
#define FREQ_2_MHZ	2000000
#define FREQ_3_MHZ	3000000
#define FREQ_4_MHZ	4000000
#define FREQ_8_MHZ	8000000
#define FREQ_12_MHZ	12000000
#define FREQ_16_MHZ	16000000
#define FREQ_20_MHZ	20000000
#define FREQ_24_MHZ	24000000

#define TICK_TOP		60000

// External Functions ---------------------------------------------------------
void sysInitTick();
uint32_t sysGetTick();

#endif /* SYS_H_ */