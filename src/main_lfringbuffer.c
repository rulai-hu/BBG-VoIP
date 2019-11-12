/**
 * Test driver.
 */

#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include <pthread.h>
#include <inttypes.h>

#include "include/lfringbuffer.h"

struct ThreadInfo
{
    RingBuffer *buffer;
    int threadNumber;
    int maxThreads;
};

#define MAX_NUMBER 40000

static void * producer(void *arg)
{
    struct ThreadInfo *info = arg;
    uint32_t number = info->threadNumber;
    RingBuffer *buffer = info->buffer;
    while (1)
    {
        if (!RingBuffer_enqueue(buffer, number))
        {
            continue;
        }

        if (number >= MAX_NUMBER)
        {
            break;
        }

        number += info->maxThreads;
    }

    printf("Producer %d exiting\n", info->threadNumber);
    return NULL;
}

static void * consumer(void *arg)
{
    RingBuffer *buffer = arg;
    uint32_t lastNumberSeen = 0;
    while (1)
    {
        uint32_t number = RingBuffer_dequeue(buffer);
        if (!number)
        {
            continue;
        }

        if (number >= MAX_NUMBER)
        {
            break;
        }

        if (lastNumberSeen == number)
        {
            printf("Found violation: %d\n", lastNumberSeen);
            break;
        }

        lastNumberSeen = number;
    }

    printf("Consumer exiting\n");
    return NULL;
}

static void testMultiThreaded()
{
    RingBuffer *ringBuffer = RingBuffer_init(1024);
    struct ThreadInfo t1 = { .buffer = ringBuffer, .threadNumber = 1, .maxThreads = 7 };
    struct ThreadInfo t2 = { .buffer = ringBuffer, .threadNumber = 2, .maxThreads = 7 };
    struct ThreadInfo t3 = { .buffer = ringBuffer, .threadNumber = 3, .maxThreads = 7 };
    struct ThreadInfo t4 = { .buffer = ringBuffer, .threadNumber = 4, .maxThreads = 7 };
    struct ThreadInfo t5 = { .buffer = ringBuffer, .threadNumber = 5, .maxThreads = 7 };
    struct ThreadInfo t6 = { .buffer = ringBuffer, .threadNumber = 6, .maxThreads = 7 };
    struct ThreadInfo t7 = { .buffer = ringBuffer, .threadNumber = 7, .maxThreads = 7 };
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    pthread_t threadId[12];
    pthread_create(&threadId[0], &attr, &producer, &t1);
    pthread_create(&threadId[1], &attr, &producer, &t2);
    pthread_create(&threadId[2], &attr, &producer, &t3);
    pthread_create(&threadId[3], &attr, &producer, &t4);
    pthread_create(&threadId[4], &attr, &producer, &t5);
    pthread_create(&threadId[5], &attr, &producer, &t6);
    pthread_create(&threadId[6], &attr, &producer, &t7);
    pthread_create(&threadId[7], &attr, &consumer, ringBuffer);
    pthread_create(&threadId[8], &attr, &consumer, ringBuffer);
    pthread_create(&threadId[9], &attr, &consumer, ringBuffer);
    pthread_create(&threadId[10], &attr, &consumer, ringBuffer);
    pthread_create(&threadId[11], &attr, &consumer, ringBuffer);
    void * bla;
    for (int i = 7 ; i < 12; ++i)
    {
        pthread_join(threadId[i], &bla);
    }

    printf("Exiting, bai\n");
}

static void testSingleThreaded()
{
    RingBuffer *buffer = RingBuffer_init(4);
    const char *t1 = "temp 1";
    const char *t2 = "temp 2";
    RingBuffer_enqueue(buffer, (char*)t1);
    RingBuffer_enqueue(buffer, (char*)t2);
    RingBuffer_enqueue(buffer, (char*)t1);
    RingBuffer_enqueue(buffer, (char*)t2);
    RingBuffer_enqueue(buffer, (char*)t2);
    assert(t1 == RingBuffer_dequeue(buffer));
    assert(t2 == RingBuffer_dequeue(buffer));
    assert(t1 == RingBuffer_dequeue(buffer));
    assert(t2 == RingBuffer_dequeue(buffer));
    assert(NULL == RingBuffer_dequeue(buffer));
    RingBuffer_destroy(buffer);
    printf("==========================\n");
}

int main(void)
{
    testSingleThreaded();
    testMultiThreaded();
    return 0;
}
