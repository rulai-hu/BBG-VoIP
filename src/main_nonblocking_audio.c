/*
 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however,
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * the can be incorporated into the canonical version. It is also
 * requested that these non-binding requests be included along with the
 * license above.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#include "include/portaudio.h"
#include "include/pa_linux_alsa.h"
#include "include/lfringbuffer.h"
#include "include/lfqueue.h"
#include "include/pink.h"

#define UNUSED(x) (void)(x)

#define FRAMES_PER_BUFFER   512
#define SAMPLE_RATE         44100
#define FRAME_TIME          ((double) FRAMES_PER_BUFFER / SAMPLE_RATE)

#define FREE_BUFFERS        300
#define FREE_NODES          300
#define BUFFER_POOL_SIZE    50
#define AUDIO_DEVICE_IDX    1

// #define FRAMES_PER_BUFFER   2048 // 46 ms per buffer @ 44khz

#define PA_SAMPLE_TYPE      paInt16
#define SILENCE             0
#define MONO                1

typedef short Sample;
typedef struct {
    lfqueue_t* sendBufferQueue;
    lfqueue_t* recvBufferQueue;
    lfqueue_t* sendBufferPool;
    lfqueue_t* recvBufferPool;
    RingBuffer* freeBuffers;
    RingBuffer* lfQueueNodePool;
} Buffers;

static void* fillRecvBuffer(void* ptr);
static void* flushSendBuffer(void* ptr);

static inline void* buf_malloc(void* pl, size_t sz);
static inline void buf_free(void* pl, void* ptr);

static int rwAudio(const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*);
static int testAudio(const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*);
static int uniformRand(int);

static PaStream*          audioStream;
static PaStreamParameters inputParams, outputParams;

static pthread_t recvThread, sendThread;

static Sample* totalSilence;

static lfqueue_t* sendBufferQueue;
static lfqueue_t* recvBufferQueue;

static lfqueue_t* sendBufferPool;
static lfqueue_t* recvBufferPool;

// static RingBuffer* bufferPool;
static RingBuffer* freeBuffers;
static RingBuffer* lfQueueNodePool;
static PinkNoise noise;

int main(void) {
    // bufferPool = RingBuffer_init(BUFFER_POOL_SIZE*10);
    freeBuffers = RingBuffer_init(FREE_BUFFERS);

    lfQueueNodePool = RingBuffer_init(FREE_NODES);
    for (int i = 0; i < FREE_NODES; i++) {
        lfqueue_cas_node_t* node = malloc(lfqueue_node_size());
        RingBuffer_enqueue(lfQueueNodePool, node);
    }

    sendBufferQueue = malloc(sizeof(lfqueue_t));
    recvBufferQueue = malloc(sizeof(lfqueue_t));
    sendBufferPool = malloc(sizeof(lfqueue_t));
    recvBufferPool = malloc(sizeof(lfqueue_t));

    lfqueue_init_mf(sendBufferQueue, lfQueueNodePool, buf_malloc, buf_free);
    // lfqueue_init(sendBufferQueue);

    lfqueue_init_mf(recvBufferQueue, lfQueueNodePool, buf_malloc, buf_free);
    // lfqueue_init(recvBufferQueue);

    lfqueue_init_mf(sendBufferPool, lfQueueNodePool, buf_malloc, buf_free);
    // lfqueue_init(sendBufferPool);

    lfqueue_init_mf(recvBufferPool, lfQueueNodePool, buf_malloc, buf_free);
    // lfqueue_init(recvBufferPool);

    Buffers buffers = {
        sendBufferQueue, recvBufferQueue, sendBufferPool, recvBufferPool, freeBuffers, lfQueueNodePool
    };

    InitializePinkNoise(&noise, 12);

    size_t bytesPerBuffer = FRAMES_PER_BUFFER * sizeof(Sample);

    // Populate the buffer pools with buffers.
    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        Sample* sendBuf = malloc(bytesPerBuffer);
        lfqueue_enq(sendBufferPool, sendBuf);

        Sample* recvBuf = malloc(bytesPerBuffer);
        lfqueue_enq(recvBufferPool, recvBuf);
    }

    for (int i = 0; i < FREE_BUFFERS; i++) {
        Sample* buffer = malloc(bytesPerBuffer);
        RingBuffer_enqueue(freeBuffers, buffer);
    }

    printf("Init FreeBuffer=%u\n", RingBuffer_count(freeBuffers));

    totalSilence = malloc(bytesPerBuffer);
    memset(totalSilence, SILENCE, bytesPerBuffer);

    PaError result = paNoError;
    const PaDeviceInfo* deviceInfo;

    result = Pa_Initialize();

    if (result != paNoError) {
        goto done;
    }

    printf("Pa_Initialize done\n");

    inputParams.device = AUDIO_DEVICE_IDX;

    deviceInfo = Pa_GetDeviceInfo(inputParams.device);

    inputParams.channelCount = MONO;
    inputParams.sampleFormat = PA_SAMPLE_TYPE;
    inputParams.suggestedLatency = deviceInfo->defaultHighInputLatency;
    inputParams.hostApiSpecificStreamInfo = NULL;

    outputParams.device = AUDIO_DEVICE_IDX;
    outputParams.channelCount = MONO;
    outputParams.sampleFormat = PA_SAMPLE_TYPE;
    outputParams.suggestedLatency = deviceInfo->defaultHighOutputLatency;
    outputParams.hostApiSpecificStreamInfo = NULL;

    result = Pa_OpenStream(&audioStream, &inputParams, &outputParams, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, rwAudio, &buffers);
    // result = Pa_OpenStream(&audioStream, NULL, &outputParams, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, testAudio, NULL);
    if (result != paNoError) {
        goto done;
    }

    PaAlsa_EnableRealtimeScheduling(&audioStream, true);

    pthread_create(&recvThread, NULL, fillRecvBuffer, &buffers);

    sleep(2); // wait for recv queue to fill up

    result = Pa_StartStream(audioStream);

    pthread_create(&sendThread, NULL, flushSendBuffer, &buffers);

    if (result != paNoError)
        goto done;

    while ((result = Pa_IsStreamActive(audioStream)) == 1) {
        printf("SendQueue=%u, RecvQueue=%u, FreeBuffers=%u, FreeNodes=%u\n",
            lfqueue_size(sendBufferQueue), lfqueue_size(recvBufferQueue),
            RingBuffer_count(buffers.freeBuffers), RingBuffer_count(buffers.lfQueueNodePool)
        );

        Pa_Sleep(100);
    }

    if (result != paNoError) {
        goto done;
    }

    result = Pa_CloseStream(audioStream);

    if (result != paNoError)
        goto done;

done:
    printf("DONE\n");
    Pa_Terminate();
    return result;
}

static int testAudio(const void* inputBuffer, void* outputBuffer, unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) {

    Sample* writePtr = (Sample*) outputBuffer;
    for (int i = 0; i < frameCount; i++) {
        *writePtr++ = (Sample) (GeneratePinkNoise(&noise) * 32600.0);;
    }

    return paContinue;
}

/**
 * Synchronized input/output to audio device.
 */
static int rwAudio(const void* inputBuffer, void* outputBuffer, unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) {

    Buffers* buffers = (Buffers*) userData;

    Sample* recvBuffer = lfqueue_single_deq(buffers->recvBufferQueue);
    Sample* recvBufferBase = recvBuffer;
    // Sample* sendBuffer = lfqueue_single_deq(buffers->sendBufferPool);
    Sample* sendBuffer = RingBuffer_dequeue(buffers->freeBuffers);
    Sample* sendBufferBase = sendBuffer;

    // Write audio data to output buffer to be played.
    // Doesn't contain anything meaningful right now.
    Sample* writePtr = (Sample*) outputBuffer;

    // Contains audio-in data
    Sample* readPtr = (Sample*) inputBuffer;

    int i;
    if (recvBuffer == NULL) {
        for (i = 0; i < frameCount; i++) {
            *writePtr++ = SILENCE;
        }
    } else {
        for (i = 0; i < frameCount; i++) {
            *writePtr++ = *recvBuffer++;
        }

        // Place the consumed buffer back into the pool to be reused.
        // lfqueue_enq(buffers->recvBufferPool, recvBuffer);
        // free(recvBuffer); // free does not work in real-time
        RingBuffer_enqueue(buffers->freeBuffers, recvBufferBase);
    }

    if (sendBuffer != NULL) {
        for (i = 0; i < frameCount; i++) {
            *sendBuffer++ = *readPtr++;
        }
    }

    // If sendBuffer = NULL, this means the pool is exhausted. This is probably
    // because the flushSendBuffer thread is not processing queued audio fast enough.
    // In this case, we just send SILENCE.
    lfqueue_enq(sendBufferQueue, sendBufferBase);

    return paContinue;
}

static int uniformRand(int modulus) {
    int x;

    int range_max = RAND_MAX - (RAND_MAX % modulus);

    do {
        x = rand();
    } while (x >= range_max);

    return x % modulus;
}

/**
 * Simulate receiving pink noise from somewhere.
 */
static void* fillRecvBuffer(void* buffers) {
    const int maxDelay = 1E6; // up to 1ms delay
    const float gain = 0.3; // quiet it down a bit
    Sample* recvBuffer;

    int simulatedDelay;

    while (1) {
        // printf("Just deq single recvBuffer\n");
        // recvBuffer = lfqueue_deq(recvBufferPool);
        recvBuffer = RingBuffer_dequeue(freeBuffers);
        if (recvBuffer == NULL) {
            // printf("Warning: dropped frame\n");
            continue;
        }

        // if no more buffers in pool, add another one
        // if (recvBuffer == NULL) {
        //     // printf("Warning: recvBufferPool empty.\n");
        //     recvBuffer = malloc(FRAMES_PER_BUFFER * sizeof(Sample));

        //     if (recvBuffer == NULL) {
        //         printf("malloc failed\n");
        //         exit(1);
        //     }
        // } else {
        //     // printf("got buffer from recvBufferPool\n");
        // }

        // recvBuffer = malloc(FRAMES_PER_BUFFER * sizeof(Sample));

        for (int i = 0; i < FRAMES_PER_BUFFER; i++) {
            recvBuffer[i] = (Sample) (GeneratePinkNoise(&noise) * 32600.0 * gain);
        }

        simulatedDelay = uniformRand(maxDelay);

        lfqueue_enq(recvBufferQueue, recvBuffer);
        // nanosleep((const struct timespec[]){{0, 11609978L + simulatedDelay}}, NULL);
        nanosleep((const struct timespec[]){{0, ((unsigned long) (FRAME_TIME*1E9)) + simulatedDelay}}, NULL);
    }

    pthread_exit(NULL);
}

/**
 * Simulate sending audio data somewhere
 */
static void* flushSendBuffer(void* buffers) {
    FILE* fp = fopen("/dev/null" , "w");
    Sample* sendBuffer;

    while (1) {
        sendBuffer = lfqueue_single_deq(sendBufferQueue);

        // If sendBuffer is NULL this means the callback drained the sendBufferPool.
        // In this edge case send silence.
        if (sendBuffer == NULL) {
            continue;
            printf("sendBuffer=NULL, sending totalSilence\n");
            // fwrite(totalSilence, sizeof(Sample), FRAMES_PER_BUFFER, fp);
        } else {
            // printf("Sending frames\n");
            fwrite(sendBuffer, sizeof(Sample), FRAMES_PER_BUFFER, fp);
            // lfqueue_enq(sendBufferPool, sendBuffer);
            RingBuffer_enqueue(freeBuffers, sendBuffer);
        }
    }

    fclose(fp);
    pthread_exit(NULL);
}

static inline void* buf_malloc(void* nodePool, size_t sz) {
    return RingBuffer_dequeue((RingBuffer*) nodePool);
}

static inline void buf_free(void* nodePool, void* ptr) {
    RingBuffer_enqueue((RingBuffer*) nodePool, ptr);
}
