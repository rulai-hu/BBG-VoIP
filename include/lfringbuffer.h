#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdlib.h>

typedef struct {
    int readPointer;
    int writePointer;
    size_t size;
    size_t count;

    void *buffer[];

} RingBuffer;

RingBuffer* RingBuffer_init(size_t size);
int RingBuffer_enqueue(RingBuffer *buffer, void *value);
void* RingBuffer_dequeue(RingBuffer *buffer);
void RingBuffer_destroy(RingBuffer *buffer);
size_t RingBuffer_count(RingBuffer *buffer);

#endif
