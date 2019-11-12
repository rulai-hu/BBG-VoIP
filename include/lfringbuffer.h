#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdlib.h>

typedef struct {
    int readPointer;
    int writePointer;
    size_t size;

    void *buffer[];

} RingBuffer;

RingBuffer* RingBuffer_init(size_t size);
int RingBuffer_enqueue(RingBuffer *buffer, void *value);
void* RingBuffer_dequeue(RingBuffer *buffer);
void RingBuffer_destroy(RingBuffer *buffer);

#endif
