/**
 * @file fio.h
 * @brief File stream I/O management for avrOS.
 *
 * This header file provides types and macros for managing file stream
 * input and output using queues for buffering. It integrates with the
 * avrOS event and FSM mechanisms for non-blocking I/O operations.
 *
 * @date 7/14/2023
 * @author John Anderson <racerxr650r@gmail.com>
 *
 * Copyright (C) 2021 by John Anderson
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

#ifndef __FIO_H
#define __FIO_H

/** @addtogroup fio_abstraction
 * @{
 */

#include "../avrOS.h"

/**
 * @brief Structure to hold file I/O buffer queues.
 *
 * This structure contains pointers to the input and output queues used for
 * buffering file I/O operations.
 */
typedef struct
{
	volatile queue_t *input;  /**< Pointer to the input queue. */
	volatile queue_t *output; /**< Pointer to the output queue. */
} fioBuffers_t;

// Macros ----------------------------------------------------------------------

/**
 * @brief Get the input queue associated with a FILE stream.
 * 
 * @param file Pointer to the FILE stream.
 * @return Pointer to the input queue.
 */
#define fioGetInputQueue(file)  (((fioBuffers_t *)(file)->buf)->input)


/**
 * @brief Get the output queue associated with a FILE stream.
 * 
 * @param file Pointer to the FILE stream.
 * @return Pointer to the output queue.
 */
#define fioGetOutputQueue(file) (((fioBuffers_t *)(file)->buf)->output)


/**
 * @brief Set the input queue for a FILE stream.
 * 
 * @param file Pointer to the FILE stream.
 * @param que  Pointer to the queue to set as the input queue.
 */
#define fioSetInputQueue(file, que)  (((fioBuffers_t *)(file)->buf)->input = (que))  // Cast removed for correctness


/**
 * @brief Set the output queue for a FILE stream.
 * 
 * @param file Pointer to the FILE stream.
 * @param que  Pointer to the queue to set as the output queue.
 */
#define fioSetOutputQueue(file, que) (((fioBuffers_t *)(file)->buf)->output = (que)) // Cast removed for correctness


/**
 * @brief Wait for data to become available in the input queue.
 * 
 * This inline function uses the avrOS event system to put the current
 * FSM in a waiting state until data is available in the input queue of the
 * specified FILE stream.
 *
 * @param file Pointer to the FILE stream.
 */
static inline void fioWaitInput(FILE *file)
{
	fioBuffers_t *buffer = (fioBuffers_t *)(file->buf);
	queWait(buffer->input, QUE_EVENT_NOT_EMPTY);
}

/**
 * @brief Wait for the output queue to become empty.
 * 
 * This inline function uses the avrOS event system to put the current
 * FSM in a waiting state until the output queue of the specified
 * FILE stream is empty.
 *
 * @param file Pointer to the FILE stream.
 */
static inline void fioWaitOutput(FILE *file)
{
	fioBuffers_t *buffer = (fioBuffers_t *)(file->buf);
	queWait(buffer->output, QUE_EVENT_EMPTY);
}

/**
 * @brief Busy-wait for data to become available in the input queue.
 *
 * This inline function actively polls the input queue until data
 * becomes available.  Use with caution as it can consume significant
 * CPU cycles.
 * 
 * @param file Pointer to the FILE stream.
 */
static inline void fioBusyWaitInput(FILE *file)
{
	fioBuffers_t *buffer = (fioBuffers_t *)(file->buf);
	while (queIsEmpty(buffer->input)); // Wait until not empty
}

/**
 * @brief Busy-wait for the output queue to become empty.
 *
 * This inline function actively polls the output queue until it becomes
 * empty.  Use with caution as it can consume significant CPU cycles.
 * 
 * @param file Pointer to the FILE stream.
 */
static inline void fioBusyWaitOutput(FILE *file)
{
	fioBuffers_t *buffer = (fioBuffers_t *)(file->buf);
	while (!queIsEmpty(buffer->output)); // Wait until empty
}

/** @} */ // end of fio_abstraction

#endif  // __FIO_H
