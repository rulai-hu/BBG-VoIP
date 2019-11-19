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

#define UNUSED(x) (void)(x)

#define PA_SAMPLE_TYPE      paInt16
#define SILENCE             0
#define MONO                1

#define FRAMES_PER_BUFFER   512
#define SAMPLE_RATE         44100
#define FRAME_PLAYBACK_TIME ((double) FRAMES_PER_BUFFER / SAMPLE_RATE)
#define FRAMEBUFFER_SIZE    FRAMES_PER_BUFFER * sizeof(Sample) * MONO // 1 channel

#define FREE_BUFFERS        300
#define FREE_NODES          600
#define AUDIO_DEVICE_IDX    1

// 1 sec playback time. Multiply by a constant n to get n secs.
#define MIN_PLAYBACK_QUEUE_LENGTH ((unsigned) SAMPLE_RATE / FRAMES_PER_BUFFER)

// Container for audio file queue
typedef struct {
    lfqueue_t* recordBufferQueue;
    lfqueue_t* playbackBufferQueue;
    RingBuffer* freeBuffers;
    RingBuffer* freeNodes;
} AudioBuffers;

AudioBuffers audioBuffers;

static void* fillPlaybackQueue(void* ptr);
static void* flushRecordQueue(void* ptr);

static int checkDevice(int);
static void clearAllQueues(void);
static void bufferPlaybackQueue(lfqueue_t*);

static inline void* bufferMalloc(void*, size_t);
static inline void bufferFree(void*, void*);

static int streamCallback(const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*);

static RingBuffer* initFreeBuffers(void);
static RingBuffer* initFreeNodes(void);
static lfqueue_t* initRecordQueue(RingBuffer*);
static lfqueue_t* initPlaybackQueue(RingBuffer*);

static PaStreamParameters inputParams, outputParams;
static PaStream* audioStream;

static pthread_t fillPlaybackQueueThread, flushRecordQueueThread, statsThread;

static const Sample totalSilence[FRAMES_PER_BUFFER] = { [0 ... (FRAMES_PER_BUFFER - 1)] = SILENCE };

static RingBuffer* freeBuffers;
static RingBuffer* freeNodes;

static lfqueue_t* recordBufferQueue;
static lfqueue_t* playbackBufferQueue;

static _Bool initialized = false;
static _Bool started = false;
static _Bool stopPlayback = false;
static _Bool stopRecording = false;

void* callbackData = NULL;

#ifdef DEBUG_AUDIO
static _Bool stopStats = false;
#endif

/**
 * This must be called once, and before Audio_start() is called.
 */
void Audio_init(int deviceIndex) {
    // It's still possible to initialize twice if multiple threads call
    // this simultaneously. Make this thread-safe if it's worth the effort?
    if (initialized) {
        return;
    }

    PaError result = Pa_Initialize();

    if (result != paNoError) {
        fprintf(stderr, "[FATAL] Audio_init: Pa_Initialize failed.\n");
        exit(1);
    }

    if (checkDevice(deviceIndex) == 0) {
        fprintf(stderr, "[FATAL] Audio_init: invalid device index %d.\n", deviceIndex);
        Pa_Terminate();
        exit(1);
    }

    stopRecording = false;
    stopPlayback = false;

    // Initialize resource pools
    freeBuffers = initFreeBuffers();
    freeNodes = initFreeNodes();

    // Initialize queues for playback/record frame buffers
    recordBufferQueue = initRecordQueue(freeNodes);
    playbackBufferQueue = initPlaybackQueue(freeNodes);

    audioBuffers.recordBufferQueue = recordBufferQueue;
    audioBuffers.playbackBufferQueue = playbackBufferQueue;
    audioBuffers.freeBuffers = freeBuffers;
    audioBuffers.freeNodes = freeNodes;

    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceIndex);

    inputParams.device = deviceIndex;
    inputParams.channelCount = MONO;
    inputParams.sampleFormat = PA_SAMPLE_TYPE;
    inputParams.suggestedLatency = deviceInfo->defaultHighInputLatency;
    inputParams.hostApiSpecificStreamInfo = NULL;

    outputParams.device = deviceIndex;
    outputParams.channelCount = MONO;
    outputParams.sampleFormat = PA_SAMPLE_TYPE;
    outputParams.suggestedLatency = deviceInfo->defaultHighOutputLatency;
    outputParams.hostApiSpecificStreamInfo = NULL;

    // Note: not a thread safe way of going about this.
    initialized = true;
}

void Audio_teardown() {
    // Free all PortAudio resources
    Pa_Terminate();

    // Misnomer: this doesn't actually destroy the queue,
    // it just clears it out and set its size to 0. So
    // basically a reset.
    lfqueue_destroy(playbackBufferQueue);
    lfqueue_destroy(recordBufferQueue);

    // Free the queues.
    free(playbackBufferQueue);
    free(recordBufferQueue);

    // Free node memory.
    lfqueue_cas_node_t* node;

    int cnt = 0;

    while ((node = RingBuffer_dequeue(freeNodes)) != NULL) {
        cnt++;
        free(node);
    }

#ifdef DEBUG_AUDIO
    printf("Freed nodes=%d\n", cnt);
#endif

    RingBuffer_destroy(freeNodes);

    // Free all frame buffers.
    FrameBuffer buf;
    cnt = 0;
    while ((buf = RingBuffer_dequeue(freeBuffers)) != NULL) {
        cnt++;
        free(buf);
    }

#ifdef DEBUG_AUDIO
    printf("Freed buffers=%d\n", cnt);
#endif

    RingBuffer_destroy(freeBuffers);

    // Set the buffer / queue pointers to NULL
    recordBufferQueue = NULL;
    playbackBufferQueue = NULL;
    freeBuffers = NULL;
    freeNodes = NULL;

    // Set all the (now invalid) audioBuffer pointers to NULL
    audioBuffers.recordBufferQueue = NULL;
    audioBuffers.playbackBufferQueue = NULL;
    audioBuffers.freeBuffers = NULL;
    audioBuffers.freeNodes = NULL;

    initialized = false;
    started = false;
}

AudioResult Audio_start(AudioProducer receiveFrameBuffer, AudioConsumer sendFrameBuffer, void* data) {
    if (started) {
        return AUDIO_ALREADY_STARTED;
    }

    callbackData = data;
    stopRecording = false;
    stopPlayback = false;
    started = true;

    AudioResult retval;

    if (!initialized) {
        return AUDIO_NOT_INITIALIZED;
    }

#ifdef DEBUG_AUDIO
    pthread_create(&statsThread, NULL, printStats, NULL);
#endif

    PaError result = Pa_OpenStream(&audioStream, &inputParams, &outputParams,
            SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, streamCallback, &audioBuffers);

    if (result != paNoError) {
        return AUDIO_OPEN_STREAM_FAIL;
    }

    PaAlsa_EnableRealtimeScheduling(&audioStream, true);

    pthread_create(&fillPlaybackQueueThread, NULL, fillPlaybackQueue, receiveFrameBuffer);

    bufferPlaybackQueue(playbackBufferQueue);

    result = Pa_StartStream(audioStream);

    if (result != paNoError) {
        retval = AUDIO_START_STREAM_FAIL;
        goto EXCEPTION;
    }

    pthread_create(&flushRecordQueueThread, NULL, flushRecordQueue, sendFrameBuffer);

    return AUDIO_OK;

EXCEPTION:
    result = Pa_CloseStream(audioStream);

    if (result != paNoError) {
        retval = AUDIO_CLOSE_STREAM_FAIL;
    }

    return retval;
}

AudioResult Audio_stop() {
    if (started) {
        return AUDIO_ALREADY_STOPPED;
    }

    stopPlayback = true;
    stopRecording = true;
    pthread_join(fillPlaybackQueueThread, NULL);
    pthread_join(flushRecordQueueThread, NULL);

    PaError res = Pa_CloseStream(audioStream);

    if (res != paNoError) {
        fprintf(stderr, "[WARN] Audio_stop: failed to close audio stream.\n");
        return AUDIO_CLOSE_STREAM_FAIL;
    }

    clearAllQueues();

#ifdef DEBUG_AUDIO
    stopStats = true;
    pthread_join(statsThread, NULL);
    printf("==============================================================\n");
    printf("Final stats:\n");
    printf("PlaybackQueue=%u, RecordQueue=%u, FreeBuffers=%u, FreeNodes=%u\n",
        lfqueue_size(playbackBufferQueue), lfqueue_size(recordBufferQueue),
        RingBuffer_count(freeBuffers), RingBuffer_count(freeNodes)
    );
#endif

    started = false;
    callbackData = NULL;

    return AUDIO_OK;
}

static int checkDevice(PaDeviceIndex idx) {
    const PaDeviceInfo* info = Pa_GetDeviceInfo(idx);

    if (info == NULL) {
        return 0;
    }

    return 1;
}

static void bufferPlaybackQueue(lfqueue_t* queue) {
    // Busy wait for queue to exceed minimum playback time.
    // This is in terms of frame buffers, not time.
    while (lfqueue_size(queue) < MIN_PLAYBACK_QUEUE_LENGTH) {
        ;
    }
}

static void clearAllQueues() {
    FrameBuffer discard;

    while ((discard = lfqueue_single_deq(playbackBufferQueue)) != NULL) {
        RingBuffer_enqueue(freeBuffers, discard);
    }

    while ((discard = lfqueue_single_deq(recordBufferQueue)) != NULL) {
        RingBuffer_enqueue(freeBuffers, discard);
    }
}

/**
 * Synchronized input/output to audio device.
 */
static int streamCallback(const void* inputBuffer, void* outputBuffer, unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) {

    UNUSED(timeInfo);
    UNUSED(statusFlags);

    AudioBuffers* buffers = (AudioBuffers*) userData;

    FrameBuffer playbackBuffer = lfqueue_single_deq(buffers->playbackBufferQueue);
    FrameBuffer playbackBufferBase = playbackBuffer;

    // Write audio data to output buffer to be played.
    // Doesn't contain anything meaningful right now.
    FrameBuffer writePtr = (FrameBuffer) outputBuffer;

    // Contains audio-in data
    FrameBuffer readPtr = (FrameBuffer) inputBuffer;

    unsigned int i;

    if (playbackBuffer == NULL) {
        for (i = 0; i < frameCount; i++) {
            *writePtr++ = SILENCE;
        }
    } else {
        for (i = 0; i < frameCount; i++) {
            *writePtr++ = *playbackBuffer++;
        }

        // Place the consumed buffer back into the pool to be reused.
        RingBuffer_enqueue(buffers->freeBuffers, playbackBufferBase);
    }

    FrameBuffer recordBuffer = RingBuffer_dequeue(buffers->freeBuffers);
    FrameBuffer recordBufferBase = recordBuffer;

    int enqv;

    if (recordBuffer != NULL) {
        for (i = 0; i < frameCount; i++) {
            *recordBuffer++ = *readPtr++;
        }

        enqv = lfqueue_enq(buffers->recordBufferQueue, recordBufferBase);
    } else {
        // This can only happen if we run out of freeBuffers,
        // which is a very bad thing.
        enqv = lfqueue_enq(buffers->recordBufferQueue, totalSilence);
    }

    if (enqv == -1) {
        printf("[FATAL] Possible buffer underflow (lfqueue_enq failed). This shouldn't happen.\n");
        exit(1);
    }

    return paContinue;
}

static void* fillPlaybackQueue(void* producer) {
    FrameBuffer buffer;
    FrameBuffer playbackSourceBuffer = malloc(FRAMEBUFFER_SIZE);
    AudioProducer produceBuffer = (AudioProducer) producer;
    AudioCallbackResult result;

    while (!stopPlayback) {
        result = produceBuffer(playbackSourceBuffer, FRAMEBUFFER_SIZE, callbackData);

        if (result == AUDIO_STOP_PLAYBACK) {
            stopPlayback = true;
            break;
        }

        buffer = RingBuffer_dequeue(freeBuffers);

        // This is bad. This means there are no more free buffers left
        // so frame processing must be deferred. This can cause stuttering
        // and high CPU usage.
        if (buffer == NULL) {
            // printf("Warning: dropped frame\n");
            continue;
        }

        // We can't just enqueue the source buffer because it's managed by
        // another thread. So we copy the contents into a buffer we manage
        // ourselves. Also this keeps the number of buffers in circulation
        // constant between the two audio threads + stream.
        memcpy(buffer, playbackSourceBuffer, FRAMEBUFFER_SIZE);

        lfqueue_enq(playbackBufferQueue, buffer);
    }

    free(playbackSourceBuffer);

    return NULL;
}

static void* flushRecordQueue(void* consumer) {
    FrameBuffer buffer;
    AudioConsumer consumeBuffer = (AudioConsumer) consumer;
    AudioCallbackResult result;

    while (!stopRecording) {
        buffer = lfqueue_single_deq(recordBufferQueue);

        // This means the recordBufferQueue is empty. A good thing,
        // because we're keeping the outgoing backlog small.
        if (buffer == NULL) {
            continue;
        }

        result = consumeBuffer(buffer, FRAMEBUFFER_SIZE, callbackData);

        // A bad thing. This means we've run out of freeBuffers to
        // buffer microphone input, so we've forever lost perfectly
        // good data. In this failure scenario, we buffer silence.
        if (buffer == totalSilence) {
            fprintf(stderr, "[WARN] Possible freeBuffer underflow. Try increasing number of free buffers.\n");
            // Do NOT return totalSilence to the free buffer pool,
            // because it's not a real buffer. It's just a constant.
        } else {
            // Return the frame buffer back to the pool to be reused.
            RingBuffer_enqueue(freeBuffers, buffer);
        }

        if (result == AUDIO_STOP_RECORDING) {
            stopRecording = true;
            break;
        }
    }

    return NULL;
}

#ifdef DEBUG_AUDIO
static void* printStats(void* ptr) {
    PaError result;
    while (!stopStats) {
        printf("PlaybackQueue=%u, RecordQueue=%u, FreeBuffers=%u, FreeNodes=%u\n",
            lfqueue_size(playbackBufferQueue), lfqueue_size(recordBufferQueue),
            RingBuffer_count(freeBuffers), RingBuffer_count(freeNodes)
        );

        Pa_Sleep(100);
    }

    return NULL;
}
#endif

static RingBuffer* initFreeBuffers() {
    RingBuffer* buffers = RingBuffer_init(FREE_BUFFERS);

    for (int i = 0; i < FREE_BUFFERS; i++) {
        FrameBuffer fbuf = malloc(FRAMEBUFFER_SIZE);
        RingBuffer_enqueue(buffers, fbuf);
    }

    return buffers;
}

static RingBuffer* initFreeNodes() {
    RingBuffer* buffer = RingBuffer_init(FREE_NODES);

    for (int i = 0; i < FREE_NODES; i++) {
        lfqueue_cas_node_t* node = malloc(lfqueue_node_size());
        RingBuffer_enqueue(buffer, node);
    }

    return buffer;
}

static lfqueue_t* initRecordQueue(RingBuffer* buffer) {
    lfqueue_t* queue = malloc(sizeof(lfqueue_t));
    lfqueue_init_mf(queue, buffer, bufferMalloc, bufferFree);

    return queue;
}

static lfqueue_t* initPlaybackQueue(RingBuffer* buffer) {
    lfqueue_t* queue = malloc(sizeof(lfqueue_t));
    lfqueue_init_mf(queue, buffer, bufferMalloc, bufferFree);

    return queue;
}

static inline void* bufferMalloc(void* pool, size_t sz) {
    UNUSED(sz);
    return RingBuffer_dequeue((RingBuffer*) pool);
}

static inline void bufferFree(void* pool, void* ptr) {
    int res = RingBuffer_enqueue((RingBuffer*) pool, ptr);

    if (res == 0) {
        fprintf(stderr, "[FATAL] bufferFree: nodePool out of space. This shouldn't happen.\n");
        exit(1);
    }
}
