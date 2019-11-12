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
// #include "include/sendbuffer.h"
#include "include/lfqueue.h"
#include "include/pink.h"

#define UNUSED(x) (void)(x)

#define BUFFER_POOL_SIZE    50
#define AUDIO_DEVICE_IDX    1

#define SAMPLE_RATE         44100
#define FRAMES_PER_BUFFER   512
// #define FRAMES_PER_BUFFER   2048 // 46 ms per buffer @ 44khz -- WARNING, this setting is actually bad in blocking mode

#define PA_SAMPLE_TYPE      paInt16
#define SILENCE             0
#define MONO                1

typedef short Sample;

static int uniformRand(int);
static void* countThings(void* ptr);
static void* fillRecvBuffer(void* ptr);
static void* flushSendBuffer(void* ptr);
static int rwAudio(const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*);
static int testAudio(const void*, void*, unsigned long, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void*);

PaStream*          audioStream;
PaStreamParameters inputParams, outputParams;

static pthread_t recvThread, sendThread, counterThread;
static pthread_mutex_t recvMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sendMutex = PTHREAD_MUTEX_INITIALIZER;

Sample* totalSilence;
char* recvBuffer;
char* sendBuffer;

lfqueue_t* sendBufferQueue;
lfqueue_t* recvBufferQueue;

lfqueue_t* sendBufferPool;
lfqueue_t* recvBufferPool;

PinkNoise noise;

// lfqueue_t *suicideBooth;
// _Bool stopSuicideBooth = false;

int main(void) {
    sendBufferQueue = malloc(sizeof(lfqueue_t));
    recvBufferQueue = malloc(sizeof(lfqueue_t));
    sendBufferPool = malloc(sizeof(lfqueue_t));
    recvBufferPool = malloc(sizeof(lfqueue_t));

    lfqueue_init(sendBufferQueue);
    lfqueue_init(recvBufferQueue);

    lfqueue_init(sendBufferPool);
    lfqueue_init(recvBufferPool);

    InitializePinkNoise(&noise, 12);
    // lfqueue_init(suicideBooth);
    // for (int i=0; i<20; i++ )
    // {
    //     float pink = GeneratePinkNoise(&noise);
    //     printf("Pink = %d\n", (Sample)(pink * 36000));
    // }

    size_t bytesPerBuffer = FRAMES_PER_BUFFER * sizeof(Sample);
    // Populate the buffer pools with buffers.
    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        // printf("Adding buffer to pool\n");
        Sample* sendBuf = malloc(bytesPerBuffer);
        lfqueue_enq(sendBufferPool, sendBuf);

        Sample* recvBuf = malloc(bytesPerBuffer);
        lfqueue_enq(recvBufferPool, recvBuf);
    }

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

    // result = Pa_OpenStream(&audioStream, &inputParams, &outputParams, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, rwAudio, NULL);
    // result = Pa_OpenStream(&audioStream, NULL, &outputParams, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, testAudio, NULL);
    result = Pa_OpenStream(&audioStream, &inputParams, &outputParams, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff, NULL, NULL);
    if (result != paNoError) {
        goto done;
    }

    // PaAlsa_EnableRealtimeScheduling(&audioStream, true);

    result = Pa_StartStream(audioStream);

    pthread_create(&recvThread, NULL, fillRecvBuffer, NULL);
    pthread_create(&counterThread, NULL, countThings, NULL);
    pthread_create(&sendThread, NULL, flushSendBuffer, NULL);

    if (result != paNoError)
        goto done;

    // while ((result = Pa_IsStreamActive(audioStream)) == 1) {
    //     printf("SendBufferQueueSize=%u\n", lfqueue_size(sendBufferQueue));
    //     printf("RecvBufferQueueSize=%u\n", lfqueue_size(recvBufferQueue));
    //     Pa_Sleep(100);
    // }

    if (result != paNoError) {
        goto done;
    }

    _Bool callFinished = false;
    // sendBuffer = malloc(bytesPerBuffer);
    // recvBuffer = malloc(bytesPerBuffer);

    while (!callFinished) {
        // pthread_mutex_lock(&recvMutex);
        recvBuffer = lfqueue_single_deq(recvBufferQueue);
        if (recvBuffer == NULL) {
            printf("recvBuffer=NULL, writing totalSilence\n");
            Pa_WriteStream(audioStream, totalSilence, FRAMES_PER_BUFFER);
            continue;
        }
        // printf("%x\n", recvBuffer);
        result = Pa_WriteStream(audioStream, recvBuffer, FRAMES_PER_BUFFER);
        free(recvBuffer);
        // pthread_mutex_unlock(&recvMutex);

        if (result != paNoError) {
            Pa_WriteStream(audioStream, totalSilence, FRAMES_PER_BUFFER);
            // printf("uh oh: %d\n", result);
            // goto done;
        }

        // pthread_mutex_lock(&sendMutex);

        // Take a free spot from the pool cuz malloc is too slow
        sendBuffer = lfqueue_single_deq(sendBufferPool);
        result = Pa_ReadStream(audioStream, sendBuffer, FRAMES_PER_BUFFER);
        lfqueue_enq(sendBufferQueue, sendBuffer);
        // pthread_mutex_unlock(&sendMutex);

        // if (result != paNoError)
        //     goto done;
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

    UNUSED(userData);

    int i;
    Sample* recvBuffer = lfqueue_single_deq(recvBufferQueue);
    Sample* sendBuffer = lfqueue_single_deq(sendBufferPool);

    // Write audio data to output buffer to be played.
    // Doesn't contain anything meaningful right now.
    Sample* writePtr = (Sample*) outputBuffer;

    // Contains audio-in data
    Sample* readPtr = (Sample*) inputBuffer;

    if (recvBuffer == NULL) {
        for (i = 0; i < frameCount; i++) {
            *writePtr++ = SILENCE;
        }
    } else {
        for (i = 0; i < frameCount; i++) {
            *writePtr++ = *recvBuffer++;
        }

        // Place the consumed buffer back into the pool to be reused.
        lfqueue_enq(recvBufferPool, recvBuffer);
    }

    if (sendBuffer != NULL) {
        for (i = 0; i < frameCount; i++) {
            *sendBuffer++ = *readPtr++;
        }
    }

    // If sendBuffer = NULL, this means the pool is exhausted. This is probably
    // because the flushSendBuffer thread is not processing queued audio fast enough.
    // In this case, we just send SILENCE.
    // lfqueue_enq(sendBufferQueue, sendBuffer);

    return paContinue;
}

static void* countThings(void* ptr) {
    while (1) {
        printf("RecvQueue=%u, SendQueue=%u, RecvPool=%u, SendPool=%u\n", lfqueue_size(recvBufferQueue), lfqueue_size(sendBufferQueue),
            lfqueue_size(recvBufferPool), lfqueue_size(sendBufferPool));
        // nanosleep((const struct timespec[]){{0, 2E9L}}, NULL);
        sleep(1);
    }

    pthread_exit(NULL);
}

// static void* suicideBoothProcessor(void* ptr) {
//     Sample* victim;

//     while (!stopSuicideBooth && lfqueue_size(suicideBooth) > 0) {
//         victim = lfqueue_single_deq(suicideBooth);

//         // it happens...
//         if (victim != NULL) {
//             free(victim);
//         }
//     }

//     pthread_exit(NULL);
// }
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
static void* fillRecvBuffer(void* ptr) {
    int gain = 1.0;
    Sample* buf;

    int simulatedNoiseAndDelayNs = uniformRand(500000);

    while (1) {
        // printf("Just deq single buf\n");
        // buf = lfqueue_single_deq_must(recvBufferPool);
        // if (buf == NULL) continue;

        // // if no more buffers in pool, add another one
        // if (buf == NULL) {
        //     // printf("Warning: recvBufferPool empty.\n");
        //     buf = malloc(FRAMES_PER_BUFFER * sizeof(Sample));

        //     if (buf == NULL) {
        //         printf("malloc failed\n");
        //         exit(1);
        //     }
        // } else {
        //     // printf("got buffer from recvBufferPool\n");
        // }
        buf = malloc(FRAMES_PER_BUFFER * sizeof(Sample));
        for (int i = 0; i < FRAMES_PER_BUFFER; i++) {
            buf[i] = (Sample) (GeneratePinkNoise(&noise) * 32600.0 * gain);
        }

        simulatedNoiseAndDelayNs = uniformRand(800000);
        // printf("Enq buf %x to recvBufferQueue\n", buf);
        lfqueue_enq(recvBufferQueue, buf);
        // printf("Sleeping for 46ms + e\n");
        nanosleep((const struct timespec[]){{0, 10850000 + simulatedNoiseAndDelayNs}}, NULL);
        // nanosleep((const struct timespec[]){{0, 4650000 + simulatedNoiseAndDelayNs}}, NULL);
    }

    pthread_exit(NULL);
}

/**
 * Simulate sending audio data somewhere
 */
static void* flushSendBuffer(void* ptr) {
    FILE* fp = fopen("/dev/null" , "w");
    Sample* sendBuffer;

    while (1) {
        sendBuffer = lfqueue_single_deq_must(sendBufferQueue);

        // If sendBuffer is NULL this means the callback drained the sendBufferPool.
        // In this edge case send silence.
        if (sendBuffer == NULL) {
            continue;
            // printf("sendBuffer=NULL, sending totalSilence\n");
            // fwrite(totalSilence, sizeof(Sample), FRAMES_PER_BUFFER, fp);
        } else {
            fwrite(sendBuffer, sizeof(Sample), FRAMES_PER_BUFFER, fp);
            lfqueue_enq(sendBufferPool, sendBuffer);
        }
    }
    fclose(fp);
    pthread_exit(NULL);
}
