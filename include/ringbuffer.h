#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdbool.h>

typedef struct {
    char* buffer;
    // index of first element in ring buffer.
    // does not necessarily equal the actual index of the element
    int head;

    // represents index of 'first' empty location at end of ring buffer
    int tail;
    int maxElements;
    int elementSize;
    int count;
    _Bool full;
} RingBuffer;

void RingBuffer_init(RingBuffer*, int, int);
void RingBuffer_destroy(RingBuffer*);
int RingBuffer_isEmpty(RingBuffer*);
void RingBuffer_enqueue(RingBuffer*, char*);
int RingBuffer_dequeue(RingBuffer*, char*);
int RingBuffer_dequeueN(RingBuffer*, char*, int);
int RingBuffer_isEmpty(RingBuffer*);

// Mainly for testing
char RingBuffer_getChar(RingBuffer*, int);
int RingBuffer_getInt(RingBuffer*, int);

#endif
