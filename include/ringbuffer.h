#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdbool.h>

// Container for the buffer and its current properties.
typedef struct {
    char* buffer;
    // Index of first element in ring buffer.
    // Does not necessarily equal the actual index of the element.
    int head;

    // Represents index of 'first' empty location at end of ring buffer
    int tail;
    int maxElements;
    int elementSize;
    int count;
    _Bool full;
} RingBuffer;

// This function initializes all fields in the buffer data structure.
// Memory is allocated which must be freed properly.
void RingBuffer_init(RingBuffer*, int, int);

// This function frees the memory allocated to the buffer.
void RingBuffer_destroy(RingBuffer*);

// This function returns whether the buffer contains any contents.
int RingBuffer_isEmpty(RingBuffer*);

// This function adds a new element to the buffer.
// If the buffer is full, the newest element will be overwritten.
void RingBuffer_enqueue(RingBuffer*, char*);

// This function removes the oldest element from the buffer, writing it to char*.
// Returns the number of items removed (0 if buffer empty, or 1 otherwise).
// If the buffer is full, the newest element will be overwritten.
int RingBuffer_dequeue(RingBuffer*, char*);

// Currently nonfunctional.
int RingBuffer_dequeueN(RingBuffer*, char*, int);

// Functions for testing purposes.
char RingBuffer_getChar(RingBuffer*, int);
int RingBuffer_getInt(RingBuffer*, int);

#endif
