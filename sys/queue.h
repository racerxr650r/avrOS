/**
 * @file queue.h
 * @brief Circular queue data structure implementation.
 *
 * This file provides functions and macros for managing circular queues.
 * It supports both data element and pointer storage.
 *
 * @date 2/19/2021
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


#ifndef QUEUE_H_
#define QUEUE_H_

// Constants ------------------------------------------------------------------
/** @brief Maximum size of a queue (limited by the uint16_t indexing). */
#define QUE_MAX_SIZE		255

// Data Types -----------------------------------------------------------------
/** @brief Enumeration of queue events. */
typedef enum
{
	QUE_EVENT_EMPTY = 1,
	QUE_EVENT_NOT_EMPTY,
	QUE_EVENT_FULL,
	QUE_EVENT_NOT_FULL
}queueEvents_t;

// Forward declarations
struct QUE_DESCRIPTOR_TYPE;
struct QUEUE_TYPE;

/** @brief Structure to hold queue statistics (optional). */
typedef struct
{
	uint32_t	in;       /**< Number of elements added to the queue. */
	uint32_t	out;      /**< Number of elements removed from the queue. */
	uint32_t	overflow; /**< Number of overflow errors (attempts to add to a full queue). */
} queStats_t;

/** @brief Structure representing a queue. */
typedef struct QUEUE_TYPE
{
	uint16_t	            			head; /**< Index of the head (next element to be removed). */
	uint16_t							tail; /**< Index of the tail (next element to be added). */
	uint16_t							max;  /**< Maximum number of elements ever in the queue. */
#ifdef QUE_STATS	
	volatile queStats_t					stats; /**< Queue statistics. */
#endif
	const struct QUE_DESCRIPTOR_TYPE 	*descr; /**< Pointer to the queue descriptor. */
} queue_t;

/** @brief Structure describing the queue (configuration and buffer). */
typedef struct QUE_DESCRIPTOR_TYPE
{
#ifdef QUE_STATS
	const char       *name;    /**< Name of the queue (for debugging/CLI). */
#endif
	volatile queue_t *queue;     /**< Pointer to the queue instance. */
	uint8_t          *buffer;    /**< Pointer to the underlying memory buffer. */
	volatile event_t *event;     /**< Pointer to the queue's event object. */
	uint16_t         capacity;   /**< Maximum capacity of the queue. */
	size_t           sizeOfElement; /**< Size of each element in bytes. */
} queDescriptor_t;

// Macros ----------------------------------------------------------------------
/**
 * @brief Macro to add and initialize a new queue.
 * 
 * This macro simplifies the process of creating a new queue, allocating 
 * memory for its buffer, and linking it to an event.  It is typically
 * used at file scope (global).
 *
 * @param queName  The name of the queue variable.
 * @param queSzElement The size of each element in the queue (in bytes).
 * @param queSz The capacity of the queue (number of elements).
 */
#ifdef QUE_STATS
#define ADD_QUEUE(queName, queSzElement, queSz) \
                  static uint8_t             CONCAT(queName,_buffer)[queSz*queSzElement]; \
                  const static queDescriptor_t CONCAT(queName,_descr); \
                  static volatile queue_t    queName = {.head = queSz, .tail = 0, .max = 0, .stats.in = 0, .stats.out = 0, .stats.overflow = 0, .descr = &CONCAT(queName,_descr)}; \
                  ADD_EVENT(queName ## _evnt); \
                  const static queDescriptor_t SECTION(QUE_TABLE) CONCAT(queName,_descr) = {.name = #queName, .queue = &queName, .buffer = CONCAT(queName,_buffer), .event = &CONCAT(queName,_evnt), .capacity = queSz, .sizeOfElement = queSzElement};
#else
#define ADD_QUEUE(queName, queSzElement, queSz)	\
                  static uint8_t             CONCAT(queName,_buffer)[queSz*queSzElement]; \
                  const static queDescriptor_t CONCAT(queName,_descr); \
                  static volatile queue_t    queName = {.head = queSz, .tail = 0, .max = 0, .descr = &CONCAT(queName,_descr)}; \
                  ADD_EVENT(queName ## _evnt); \
                  const static queDescriptor_t SECTION(QUE_TABLE) CONCAT(queName,_descr) = {.queue = &queName, .buffer = CONCAT(queName,_buffer), .event = &CONCAT(queName,_evnt), .capacity = queSz, .sizeOfElement = queSzElement};
#endif

// External Functions ---------------------------------------------------------
/**
 * @brief Check if the queue is empty.
 *
 * @param que Pointer to the queue.
 * @return True if the queue is empty, false otherwise.
 */
static inline bool queIsEmpty(volatile queue_t *que)
{
	return(que->head == que->descr->capacity?true:false);
}

/**
 * @brief Check if the queue is full.
 *
 * @param que Pointer to the queue.
 * @return True if the queue is full, false otherwise.
 */
static inline bool queIsFull(volatile queue_t *que)
{
	return(que->tail == que->descr->capacity?true:false);
}

static inline uint8_t queGetCapacity(volatile queue_t *que)
{
	return(que->descr->capacity);
}

static inline volatile event_t *queGetEvent(volatile queue_t *que)
{
	return(que->descr->event);
}

//#ifdef QUE_STATS
static inline uint32_t queGetMaxSize(volatile queue_t *que)
{
	return(que->max);
}

#ifdef QUE_STATS
static inline uint32_t queGetIn(volatile queue_t *que)
{
	return(que->stats.in);
}

static inline uint32_t queGetOut(volatile queue_t *que)
{
	return(que->stats.out);
}

static inline uint32_t queGetOverflow(volatile queue_t *que)
{
	return(que->stats.overflow);
}

#endif

/**
 * @brief Get the size of the queue.
 * 
 * Returns the number of elements currently in the queue.
 *
 * @param que Pointer to the queue.
 * @return The number of elements in the queue.
 */
extern uint16_t queGetSize(volatile queue_t *que);

/**
 * @brief Get an element from the queue.
 *
 * Retrieves an element from the head of the queue.  If the queue
 * is empty, the function returns false and the element is not modified.
 *
 * @param que Pointer to the queue.
 * @param element Pointer to the location where the retrieved element 
 *                should be stored.
 * @return True if an element was successfully retrieved, false otherwise.
 */
extern bool queGet(volatile queue_t *que, void *element);
static inline bool queGetByte(volatile queue_t *que, uint8_t *byte)
{
	return(queGet(que, byte));
}
static inline bool queGetWord(volatile queue_t *que, uint16_t *word)
{
	return(queGet(que, word));
}
/**
 * @brief Get a pointer from the queue.
 * 
 * This is a convenience macro to retrieve a pointer from the queue. 
 * It uses queGet() internally.
 *
 * @param que Pointer to the queue.
 * @param ptr A pointer to a pointer variable where the retrieved 
 *            pointer will be stored.
 * @return True if a pointer was successfully retrieved, false otherwise.
 */
static inline bool queGetPtr(volatile queue_t *que, void **ptr)
{
	return(queGetWord(que,(uint16_t *)ptr));
}

/**
 * @brief Put an element into the queue.
 *
 * Adds an element to the tail of the queue. If the queue is full, 
 * the function returns false and the element is not added.
 *
 * @param que Pointer to the queue.
 * @param element Pointer to the element to be added.
 * @return True if the element was successfully added, false otherwise.
 */
extern bool quePut(volatile queue_t *que, void *element);
static inline bool quePutByte(volatile queue_t *que, uint8_t byte)
{
	return(quePut(que, (void *)&byte));
}
static inline bool quePutWord(volatile queue_t *que, uint16_t word)
{
	return(quePut(que, (void *)&word));
}
/**
 * @brief Put a pointer into the queue.
 * 
 * This is a convenience macro to add a pointer to the queue. It
 * uses quePut() internally.
 *
 * @param que Pointer to the queue.
 * @param ptr The pointer to be added to the queue.
 */
static inline bool quePutPtr(volatile queue_t *que, void *ptr)
{
	return(quePutWord(que,(uint16_t)ptr));
}

#endif /* QUEUE_H_ */
