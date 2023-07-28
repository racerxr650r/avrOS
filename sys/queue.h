/*
 * queue.h
 *
 * Created: 3/8/2021 10:01:01 PM
 *  Author: admin
 */ 


#ifndef QUEUE_H_
#define QUEUE_H_

// Constants ------------------------------------------------------------------
#define QUE_MAX_SIZE		255

// Data Types -----------------------------------------------------------------
typedef enum
{
	QUE_EVENT_EMPTY = 1,
	QUE_EVENT_NOT_EMPTY,
	QUE_EVENT_FULL,
	QUE_EVENT_NOT_FULL
}queueEvents_t;

typedef struct
{
	uint32_t	in, out, underflow, overflow, max;
}QueueStats_t;

typedef struct
{
	uint8_t	size,head,tail;
}QueueState_t;

typedef struct
{
	const char				*name;
	volatile QueueState_t	*queue;
	char					*buffer;
/*	volatile event_t        *event;*/
#ifdef QUE_STATS
	QueueStats_t	*stats;
#endif
}Queue_t;

// Macros ----------------------------------------------------------------------
#ifdef QUE_STATS
#define ADD_QUEUE(queName, queSz)	static char			CONCAT(queName,_buffer)[queSz]; \
									volatile static QueueState_t	CONCAT(queName,_state) = {.size = queSz, .head = queSz, .tail = 0}; \
									static QueueStats_t	CONCAT(queName,_stats) = {.in = 0, .out = 0, .underflow = 0, .overflow = 0, .max = 0}; \
									/*ADD_EVENT(queName ## _event);*/ \
									const static Queue_t SECTION(QUE_TABLE) queName = {.name = #queName, .queue = &CONCAT(queName,_state), .buffer = CONCAT(queName,_buffer), .stats = &CONCAT(queName,_stats), /*.event = &CONCAT(queName,_event)*/};
#else
#define ADD_QUEUE(queName, queSz)	static char			CONCAT(queName,_buffer)[queSz]; \
									volatile static QueueState_t	CONCAT(queName,_state) = {.size = queSz, .head = queSz, .tail = 0}; \
									const static Queue_t SECTION(QUE_TABLE) queName = {.name = #queName, .queue = &CONCAT(queName,_state), .buffer = CONCAT(queName,_buffer)};
#endif

// External Functions ---------------------------------------------------------
//extern bool queInit(Queue_t *que,char *buffer, uint8_t size);
extern bool queEmpty(const Queue_t *que);
extern bool queFull(const Queue_t *que);
extern uint8_t queSize(const Queue_t *que);
extern uint8_t queCount(const Queue_t *que);
extern bool queGet(const Queue_t *que, char *ch);
extern bool quePut(const Queue_t *que, char ch);
#ifdef QUE_STATS
extern uint8_t queMax(const Queue_t *que);
#endif
#endif /* QUEUE_H_ */
