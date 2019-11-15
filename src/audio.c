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

#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "include/pa_linux_alsa.h"
#include "include/lfringbuffer.h"
#include "include/portaudio.h"
#include "include/lfqueue.h"
#include "include/audio.h"
// #include "include/pink.h"

#define UNUSED(x) (void)(x)

#define PA_SAMPLE_TYPE      paInt16
#define SILENCE             0
#define MONO                1

#define FRAMES_PER_BUFFER   512
#define SAMPLE_RATE         44100
#define FRAME_PLAYBACK_TIME ((double) FRAMES_PER_BUFFER / SAMPLE_RATE)
#define FRAMEBUFFER_SIZE    FRAMES_PER_BUFFER * sizeof(Sample) * MONO // 1 channel

#define FREE_BUFFERS        300
#define FREE_NODES          300
#define BUFFER_POOL_SIZE    50
#define AUDIO_DEVICE_IDX    1

// 1 sec playback time. Multiply by a constant n to get n secs.
#define MIN_RECV_QUEUE_LENGTH ((unsigned) SAMPLE_RATE / FRAMES_PER_BUFFER)

typedef struct {
    lfqueue_t* sendBufferQueue;
    lfqueue_t* recvBufferQueue;
    RingBuffer* freeBuffers;
    RingBuffer* lfQueueNodePool;
} Buffers;

static void* fillRecvBuffer(void* ptr);
static void* flushSendBuffer(void* ptr);

static void bufferRecvQueue(void);

static inline void* bufferMalloc(void*, size_t);
static inline void bufferFree(void*, void*);

static int rwAudio(const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*);

static void sendFrameBuffer(FrameBuffer);
static void receiveFrameBuffer(FrameBuffer);

static RingBuffer* initFreeBuffers(void);
static RingBuffer* initFreeNodes(void);
static lfqueue_t* initSendQueue(RingBuffer*);
static lfqueue_t* initRecvQueue(RingBuffer*);

static PaStreamParameters inputParams, outputParams;
static PaStream* audioStream;

static pthread_t recvThread, sendThread;

static const Sample totalSilence[FRAMES_PER_BUFFER] = { [0 ... (FRAMES_PER_BUFFER - 1)] = SILENCE };

static RingBuffer* freeBuffers;
static RingBuffer* freeNodes;

static lfqueue_t* sendBufferQueue;
static lfqueue_t* recvBufferQueue;

static _Bool initialized = false;
static _Bool stopAudio = false;

// static PinkNoise noise;

/**
 * This must be called once, and before Audio_start() is called.
 */
void Audio_init() {
    // Still possible to initialize twice if multiple threads call
    // this at the same time. Maybe make this thread-safe if it's
    // worth the effort?
    if (initialized) {
        return;
    }

    stopAudio = false;

    // Initialize resource pools
    freeBuffers = initFreeBuffers();
    freeNodes = initFreeNodes();

    // Initialize queues for send/recv of frame buffers
    sendBufferQueue = initSendQueue(freeNodes);
    recvBufferQueue = initRecvQueue(freeNodes);

    PaError result = Pa_Initialize();

    if (result != paNoError) {
        fprintf(stderr, "Audio_init: Pa_Initialize failed.\n");
        exit(1);
    }

    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(AUDIO_DEVICE_IDX);

    inputParams.device = AUDIO_DEVICE_IDX;
    inputParams.channelCount = MONO;
    inputParams.sampleFormat = PA_SAMPLE_TYPE;
    inputParams.suggestedLatency = deviceInfo->defaultHighInputLatency;
    inputParams.hostApiSpecificStreamInfo = NULL;

    outputParams.device = AUDIO_DEVICE_IDX;
    outputParams.channelCount = MONO;
    outputParams.sampleFormat = PA_SAMPLE_TYPE;
    outputParams.suggestedLatency = deviceInfo->defaultHighOutputLatency;
    outputParams.hostApiSpecificStreamInfo = NULL;

    // Note: not a thread safe way of going about this.
    initialized = true;
}

void Audio_teardown() {
    Pa_Terminate();
}

AudioResult Audio_start(AudioProducer receiveFrameBuffer, AudioConsumer sendFrameBuffer) {
    stopAudio = false;

    AudioResult retval;

    if (!initialized) {
        return AUDIO_NOT_INITIALIZED;
    }

    Buffers bufs = {
        sendBufferQueue, recvBufferQueue, freeBuffers, freeNodes
    };

    PaError result = Pa_OpenStream(&audioStream, &inputParams, &outputParams,
            SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, rwAudio, &bufs);

    if (result != paNoError) {
        return AUDIO_OPEN_STREAM_FAIL;
    }

    PaAlsa_EnableRealtimeScheduling(&audioStream, true);

    pthread_create(&recvThread, NULL, fillRecvBuffer, receiveFrameBuffer);

    bufferRecvQueue();

    result = Pa_StartStream(audioStream);

    if (result != paNoError) {
        retval = AUDIO_START_STREAM_FAIL;
        goto EXCEPTION;
    }

    pthread_create(&sendThread, NULL, flushSendBuffer, sendFrameBuffer);

    return AUDIO_OK;

EXCEPTION:
    result = Pa_CloseStream(audioStream);

    if (result != paNoError) {
        retval = AUDIO_CLOSE_STREAM_FAIL;
    }

    return retval;
}

AudioResult Audio_stop() {
    stopAudio = true;

    pthread_join(recvThread, NULL);
    pthread_join(sendThread, NULL);

    PaError res = Pa_CloseStream(audioStream);

    if (res != paNoError) {
        return AUDIO_CLOSE_STREAM_FAIL;
    }

    return AUDIO_OK;
}

static void receiveFrameBuffer(FrameBuffer buffer) {

}

static void sendFrameBuffer(FrameBuffer buffer) {

}

static void bufferRecvQueue() {
    // Busy wait for queue to exceed minimum playback time.
    // This is in terms of frame units, not time.
    while (lfqueue_size(sendBufferQueue) < MIN_RECV_QUEUE_LENGTH) {
        ;
    }
}

/**
 * Synchronized input/output to audio device.
 */
static int rwAudio(const void* inputBuffer, void* outputBuffer, unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) {

    UNUSED(timeInfo);
    UNUSED(statusFlags);

    Buffers* buffers = (Buffers*) userData;

    FrameBuffer recvBuffer = lfqueue_single_deq(buffers->recvBufferQueue);
    FrameBuffer recvBufferBase = recvBuffer;

    // Write audio data to output buffer to be played.
    // Doesn't contain anything meaningful right now.
    FrameBuffer writePtr = (FrameBuffer) outputBuffer;

    // Contains audio-in data
    FrameBuffer readPtr = (FrameBuffer) inputBuffer;

    unsigned int i;

    if (recvBuffer == NULL) {
        for (i = 0; i < frameCount; i++) {
            *writePtr++ = SILENCE;
        }
    } else {
        for (i = 0; i < frameCount; i++) {
            *writePtr++ = *recvBuffer++;
        }

        // Place the consumed buffer back into the pool to be reused.
        RingBuffer_enqueue(buffers->freeBuffers, recvBufferBase);
    }

    FrameBuffer sendBuffer = RingBuffer_dequeue(buffers->freeBuffers);
    FrameBuffer sendBufferBase = sendBuffer;

    if (sendBuffer != NULL) {
        for (i = 0; i < frameCount; i++) {
            *sendBuffer++ = *readPtr++;
        }

        lfqueue_enq(sendBufferQueue, sendBufferBase);
    } else {
        // This can only happen if we run out of freeBuffers,
        // which is a very bad thing.
        lfqueue_enq(sendBufferQueue, totalSilence);
    }

    return paContinue;
}

static void* fillRecvBuffer(void* recvFn) {
    FrameBuffer recvDestination;
    FrameBuffer recvSource;
    AudioProducer getBuffer = (AudioProducer) recvFn;

    while (!stopAudio) {
        getBuffer(recvSource, FRAMEBUFFER_SIZE);
        recvDestination = RingBuffer_dequeue(freeBuffers);

        // This is bad. This means there are no more free buffers left
        // so frame processing must be deferred. This can cause stuttering
        // and high CPU usage.
        if (recvDestination == NULL) {
            // printf("Warning: dropped frame\n");
            continue;
        }

        memcpy(recvDestination, recvSource, FRAMEBUFFER_SIZE);

        lfqueue_enq(recvBufferQueue, recvDestination);
    }

    pthread_exit(NULL);
}

/**
 * Simulate sending audio data somewhere
 */
static void* flushSendBuffer(void* sendFn) {
    FrameBuffer source;
    AudioConsumer sendBuffer = (AudioConsumer) sendFn;

    while (!stopAudio) {
        source = lfqueue_single_deq(sendBufferQueue);

        // This means the sendBufferQueue is empty. A good thing,
        // because we're keeping the outgoing backlog small.
        if (source == NULL) {
            continue;
        }

        // A bad thing. This means we've run out of freeBuffers to
        // buffer microphone input, so we've forever lost perfectly
        // good data. In this failure scenario, we buffer silence.
        if (source == totalSilence) {
            fprintf(stderr, "Warning: possible freeBuffer underflow. Try increasing number of free buffers.\n");
            sendBuffer(source, FRAMEBUFFER_SIZE);
            // Do NOT return totalSilence to the free buffer pool,
            // because it's not a real buffer. It's just a constant.
        } else {
            sendBuffer(source, FRAMEBUFFER_SIZE);
            // Return the frame buffer back to the pool to be reused.
            RingBuffer_enqueue(freeBuffers, source);
        }
    }

    pthread_exit(NULL);
}

static RingBuffer* initFreeBuffers() {
    RingBuffer* freeBuffers = RingBuffer_init(FREE_BUFFERS);

    for (int i = 0; i < FREE_BUFFERS; i++) {
        Sample* buffer = malloc(FRAMEBUFFER_SIZE);
        RingBuffer_enqueue(freeBuffers, buffer);
    }

    return freeBuffers;
}

static RingBuffer* initFreeNodes() {
    RingBuffer* freeNodes = RingBuffer_init(FREE_NODES);

    for (int i = 0; i < FREE_NODES; i++) {
        lfqueue_cas_node_t* node = malloc(lfqueue_node_size());
        RingBuffer_enqueue(freeNodes, node);
    }

    return freeNodes;
}

static lfqueue_t* initSendQueue(RingBuffer* freeNodes) {
    lfqueue_t* sendQueue = malloc(sizeof(lfqueue_t));
    lfqueue_init_mf(sendQueue, freeNodes, bufferMalloc, bufferFree);

    return sendQueue;
}

static lfqueue_t* initRecvQueue(RingBuffer* freeNodes) {
    lfqueue_t* recvQueue = malloc(sizeof(lfqueue_t));
    lfqueue_init_mf(recvBufferQueue, freeNodes, bufferMalloc, bufferFree);

    return recvQueue;
}

static inline void* bufferMalloc(void* nodePool, size_t sz) {
    UNUSED(sz);
    return RingBuffer_dequeue((RingBuffer*) nodePool);
}

static inline void bufferFree(void* nodePool, void* ptr) {
    int res = RingBuffer_enqueue((RingBuffer*) nodePool, ptr);

    if (res == 0) {
        fprintf(stderr, "[!!!] FATAL: nodePool out of space. This shouldn't happen.\n");
        exit(1);
    }
}
