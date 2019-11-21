/**
 * Implements a simple lock-free circular buffer that can be used as a bounded queue.
 *
 * This implementation is safe for multiple producers and consumers.
 *
 * Dennis Futselaar (C) 2016. Released under the 2-clause BSD license.
 */

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdlib.h>

// Container for the buffer and its current properties.
typedef struct {
    int readPointer;
    int writePointer;
    size_t size;
    size_t count;

    void *buffer[];

} RingBuffer;

// This function initializes all fields in the buffer data structure.
// Memory is allocated which must be freed properly.
RingBuffer* RingBuffer_init(size_t size);

// This function adds a new element to the buffer.
// If the buffer is full, the newest element will be overwritten.
int RingBuffer_enqueue(RingBuffer *buffer, void *value);

// This function removes the oldest element from the buffer, writing it to char*.
// Returns the number of items removed (0 if buffer empty, or 1 otherwise).
// If the buffer is full, the newest element will be overwritten.
void* RingBuffer_dequeue(RingBuffer *buffer);

// This function frees the memory allocated to the buffer.
void RingBuffer_destroy(RingBuffer *buffer);

// This function returns the number of items currently in the buffer.
// Not thread-safe; for debugging purposes only.
size_t RingBuffer_count(RingBuffer *buffer);

#endif
