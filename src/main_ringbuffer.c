#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/ringbuffer.h"

#define MAX_ELEMENTS 3
#define ELEMENT_SIZE 4 // size of int32

void printBuf(RingBuffer*);
// int inRange(int, int, int);
// int outRange(int, int, int);

void printBuf(RingBuffer* ring) {
    printf("============\n");
    printf("Count=%d\n", ring->count);
    printf("HeadIdx=%d\n", ring->head);
    printf("TailIdx=%d\n", ring->tail);

    if (RingBuffer_isEmpty(ring)) {
        for (int i = 0; i < ring->maxElements; i++) {
            printf("X ");
        }
        printf("\n");
        goto END;
    }

    if (ring->head == ring->tail) {
        for (int i = 0; i < ring->maxElements; i++) {
            if (ring->elementSize == sizeof(int)) {
                printf("%d ", RingBuffer_getInt(ring, i));
            } else {
                printf("%d ", RingBuffer_getChar(ring, i));
            }
            // printf("%d ", ring->buffer[i]);
        }
        printf("\n");
        goto END;
    }

    int a, b;

    a = ring->head;
    b = ring->tail;

    for (int i = 0; i < ring->maxElements; i++) {
        if (a < b) {
            if (!(a <= i && i < b)) {
                printf("X ");
                continue;
            }
        } else { // b < a
            if (b <= i && i < a) {
                printf("X ");
                continue;
            }
        }

        if (ring->elementSize == sizeof(int)) {
            printf("%d ", RingBuffer_getInt(ring, i));
        } else {
            printf("%d ", RingBuffer_getChar(ring, i));
        }
        // printf("%d ", ring->buffer[i]);
    }
    printf("\n");
END:
    printf("============\n");
}

// H
// T
// X X X
// 0 1 2

// H   T
// 1 2 X
// 0 1 2

//   T   H
// 3 X X 2 4
// 0 1 2 3 4

// T H
// X 2 5
// 0 1 2

//   H T
// X 2 X
// 0 1 2

int main(void) {
    printf("Initializing RingBuffer with maxElements=%d, %d byte elements\n", MAX_ELEMENTS, ELEMENT_SIZE);
    printf("Input 'dq' to dequeue next element, input a number to enqueue it.\n");
    char input[20];
    RingBuffer* buf = malloc(sizeof(RingBuffer));
    int v;
    int outputBufferSize = 30;
    int outputBuffer[outputBufferSize];

    RingBuffer_init(buf, MAX_ELEMENTS, ELEMENT_SIZE);

    while (1) {
        printf("Input=");
        scanf("%s", input);

        if (strncmp(input, "dqn", 3) == 0) {
            memset(outputBuffer, 0, outputBufferSize*sizeof(int));
            int n = atoi(&input[3]);
            int res = RingBuffer_dequeueN(buf, (char*)outputBuffer, n);
            if (res) {
                printf("Dequeued N=%d\n", n);
            } else {
                printf("Nothing to dequeue\n");
            }
        } else if (strncmp(input, "dq", 2) == 0) {
            int res = RingBuffer_dequeue(buf, (char*)(&v));
            if (res) {
                printf("Dequeued %d\n", v);
            } else {
                printf("Nothing to dequeue\n");
            }
        } else {
            int x = (int) atoi(input);
            RingBuffer_enqueue(buf, (char*)(&x));
            printf("Enqueued %d\n", x);
        }

        printBuf(buf);
        printf("\n");
    }
}
