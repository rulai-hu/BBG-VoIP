#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "../include/ringbuffer.h"

static char* getBufferLoc(RingBuffer*, int);

void RingBuffer_init(RingBuffer* ring, int maxElements, int elementSize) {
    ring->buffer = malloc(maxElements * elementSize);
    ring->maxElements = maxElements;
    ring->elementSize = elementSize;
    ring->count = 0;
    ring->head = 0;
    ring->tail = 0;
    ring->full = false;
}

void RingBuffer_destroy(RingBuffer* ring) {
    free(ring->buffer);
}

int RingBuffer_isEmpty(RingBuffer* ring) {
    return ring->count == 0;
}

static char* getBufferLoc(RingBuffer* ring, int i) {
    return ring->buffer + (i * ring->elementSize);
    // return ring->buffer + ((i % ring->maxElements) * ring->elementSize);
}

/**
 * If the buffer is full, this will overwrite the newest element.
 */
void RingBuffer_enqueue(RingBuffer* ring, char* elementPtr) {
    if (ring->full) {
        int idx = ring->tail - 1;
        if (idx < 0) {
            idx += ring->maxElements;
        }
        // just overwrite the tail element, don't change anything else
        memcpy(getBufferLoc(ring, idx), elementPtr, ring->elementSize);
    } else {
        // since the buffer has more space, we increment the tail index and
        // put the new element in that location.
        memcpy(getBufferLoc(ring, ring->tail), elementPtr, ring->elementSize);
        ring->tail = (ring->tail + 1) % ring->maxElements;

        // the buffer is full if the next index is the head index
        ring->full = ring->tail == ring->head;

        ring->count++;
    }
}

char RingBuffer_getChar(RingBuffer* ring, int idx) {
    char res;
    memcpy(&res, getBufferLoc(ring, idx), 1);
    return res;
}

int RingBuffer_getInt(RingBuffer* ring, int idx) {
    assert(ring->elementSize % sizeof(int) == 0);
    int res;
    memcpy(&res, getBufferLoc(ring, idx), sizeof(int));
    return res;
}
// N=3
// 0 1 2
// 1 1 1
// 1 2 0 : tail vals

/**
 * Semantically, pop will remove the NEWEST element in the buffer, ie.
 * the latest data arriving.
 */
// void RingBuffer_pop(RingBuffer* ring, char* e) {
//     if (e != NULL) {
//         memcpy(e, getBufferLoc(ring, ring->tail), ring->elementSize);
//     }

//     int idx = ring->tail;

//     // if the tail is -1, wrap around to N - 1
//     if (--idx < 0) {
//         idx += ring->maxElements;
//     }

//     ring->tail = idx;
//     ring->full = false;
//     ring->count--;
// }

/**
 * Remove the OLDEST piece of data from the buffer.
 */
int RingBuffer_dequeue(RingBuffer* ring, char* e) {
    if (ring->count == 0) {
        return 0;
    }

    if (e != NULL) {
        memcpy(e, getBufferLoc(ring, ring->head), ring->elementSize);
    }

    ring->head = (ring->head + 1) % ring->maxElements;
    ring->full = false;
    ring->count--;

    return 1;
}

/**
 * This doesn't work. Fix later.
 */
int RingBuffer_dequeueN(RingBuffer* ring, char* e, int n) {
    if (n == 0 || ring->count == 0 || n > ring->maxElements) {
        return 0;
    }

    int head = ring->head;
    int N = ring->maxElements;
    // int end = head + n;
    int end = (head + n) % N;
    int numBytes;
    int elementsRemoved;

    // if (head < end) {
    //     if (!(head <= i && i < b)) {
    //     }
    // } else { // b < a
    //     if (b <= i && i < a) {
    //     }
    // }

    // if (end < ring->maxElements) {
    //     if (end >= ring->tail) {

    //     }
    // } else {
    //     end %= N;

    //     // doesn't quite work right. like if n is really big
    //     if (end >= ring->tail) {
    //         end = ring->tail - 1;

    //         if (end < 0) {
    //             end += N;
    //         }
    //     }
    // }
    if (end >= ring->tail) {
        end = ring->tail - 1;

        if (end < 0) {
            end += N;
        }
    }

//   E T H
// 0 1 2 3 4

//   H   E T
// 0 1 2 3 4

    // if end < head, then we memcpy the two disjoint regions
    printf("n=%d, head=%d, end=%d\n", n, head, end);
    if (end < head) {
        printf("end < head\n");
        numBytes = ring->elementSize * (N - head);
        elementsRemoved = N - head + end + 1;
        printf("elementsRemoved=%d\n", elementsRemoved);
        memcpy(e, getBufferLoc(ring, head), numBytes);
        memcpy(e + numBytes, getBufferLoc(ring, 0), ring->elementSize * (end + 1));
    } else {
        printf("end >= head\n");
        elementsRemoved = end - head + 1;

        // if (elementsRemoved == 0) {
        //     elementsRemoved++;
        // }

        printf("elementsRemoved=%d\n", elementsRemoved);
        numBytes = elementsRemoved * ring->elementSize;
        memcpy(e, getBufferLoc(ring, head), numBytes);
    }

    ring->head = (head + n) % ring->maxElements;
    ring->full = false;
    ring->count -= elementsRemoved;

    return 1;
}
