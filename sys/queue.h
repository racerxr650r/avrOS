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

struct QUE_DESCRIPTOR_TYPE;
struct QUEUE_TYPE;

typedef struct QUE_STATS_TYPE
{
	uint32_t	in, out, underflow, overflow, max;
}queStats_t;

typedef struct QUEUE_TYPE
{
	uint16_t	            head,tail;
	const struct QUE_DESCRIPTOR_TYPE *descr;
}queue_t;

typedef struct QUE_DESCRIPTOR_TYPE
{
	const char       *name;
	volatile queue_t *queue;
	uint8_t          *buffer;
	volatile event_t *event;
	uint16_t         capacity;
	size_t           sizeOfElement;
#ifdef QUE_STATS
	volatile queStats_t	*stats;
#endif
}queDescriptor_t;

// Macros ----------------------------------------------------------------------
#ifdef QUE_STATS
#define ADD_QUEUE(queName, queSzElement, queSz) \
                  static uint8_t             CONCAT(queName,_buffer)[queSz*queSzElement]; \
                  const static queDescriptor_t CONCAT(queName,_descr); \
                  static volatile queue_t    queName = {.head = queSz, .tail = 0, .descr = &CONCAT(queName,_descr)}; \
                  static volatile queStats_t CONCAT(queName,_stats) = {.in = 0, .out = 0, .underflow = 0, .overflow = 0, .max = 0}; \
                  ADD_EVENT(queName ## _event); \
                  const static queDescriptor_t SECTION(QUE_TABLE) CONCAT(queName,_descr) = {.name = #queName, .queue = &queName, .buffer = CONCAT(queName,_buffer), .stats = &CONCAT(queName,_stats), .event = &CONCAT(queName,_event), .capacity = queSz, .sizeOfElement = queSzElement};
#else
#define ADD_QUEUE(queName, queSzElement, queSz)	\
                  static uint8_t			CONCAT(queName,_buffer)[queSz*queSzElement]; \
                  volatile static queue_t	CONCAT(queName,_state) = {.head = queSz, .tail = 0}; \
                  ADD_EVENT(queName ## _event); \
                  const static queDescriptor_t SECTION(QUE_TABLE) queName = {.name = #queName, .queue = &CONCAT(queName,_state), .buffer = CONCAT(queName,_buffer), .event = &CONCAT(queName,_event), .capacity = queSz, .sizeOfElement = queSzElement};
#endif

// External Functions ---------------------------------------------------------
static inline bool queIsEmpty(volatile queue_t *que)
{
	return(que->head == que->descr->capacity?true:false);
}

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

#ifdef QUE_STATS
static inline uint32_t queGetMaxSize(volatile queue_t *que)
{
	return(que->descr->stats->max);
}

static inline uint32_t queGetIn(volatile queue_t *que)
{
	return(que->descr->stats->in);
}

static inline uint32_t queGetOut(volatile queue_t *que)
{
	return(que->descr->stats->out);
}

static inline uint32_t queGetOverflow(volatile queue_t *que)
{
	return(que->descr->stats->overflow);
}

static inline uint32_t queGetUnderflow(volatile queue_t *que)
{
	return(que->descr->stats->underflow);
}
#endif
extern uint8_t queGetSize(volatile queue_t *que);
extern bool queGet(volatile queue_t *que, void *element);
static inline bool queGetByte(volatile queue_t *que, uint8_t *byte)
{
	return(queGet(que, byte));
};
static inline bool queGetWord(volatile queue_t *que, uint16_t *word)
{
	return(queGet(que, word));
}
static inline bool queGetPtr(volatile queue_t *que, void **ptr)
{
	return(queGetWord(que,(uint16_t *)*ptr));
};

extern bool quePut(volatile queue_t *que, void *element);
static inline bool quePutByte(volatile queue_t *que, uint8_t byte)
{
	return(quePut(que, (void *)&byte));
}
static inline bool quePutWord(volatile queue_t *que, uint16_t word)
{
	return(quePut(que, (void *)&word));
}
static inline bool quePutPtr(volatile queue_t *que, void *ptr)
{
	return(quePutWord(que,(uint16_t)ptr));
};

#endif /* QUEUE_H_ */
