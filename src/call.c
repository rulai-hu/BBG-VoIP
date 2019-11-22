// This is non-portable, only works on Linux 2.6+
// We need for poll event POLLRDHUP
#define _GNU_SOURCE

#ifdef __APPLE__
// Get rid of linter errors for developing in a Mac environment
#define POLLRDHUP 0x0000
#endif

#include <sys/socket.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <poll.h>

#include "include/connection.h"
#include "include/audio.h"
#include "include/trek.h"
#include "include/call.h"

static const Sample totalSilence[FRAMES_PER_BUFFER] = { [0 ... (FRAMES_PER_BUFFER - 1)] = SILENCE };
// static PinkNoise noise;

static AudioCallbackResult receiveDatagram(FrameBuffer, const size_t, void*);
static AudioCallbackResult sendDatagram(FrameBuffer, const size_t, void*);
// static void setError(int);
static void createCallThread(Connection*);
static void* beginCall(void*);

void Call_accept(Connection* conn) {
    int handshake = CALL_ACCEPT;
    send(conn->socket, &handshake, sizeof(handshake), 0);
}

void Call_reject(Connection* conn) {
    int handshake = CALL_REJECT;
    send(conn->socket, &handshake, sizeof(handshake), 0);
}

CallResult Call_begin(Connection* conn) {
    CallResult handshake;
    ssize_t bytesReceived = recv(conn->socket, &handshake, sizeof(handshake), 0);

    if (handshake == CALL_REJECT) {
        printf("Call rejected.\n");
        return CALL_FAIL;
    }

    Trek_play(conn);

    printf("Handshake success, call accepted.\n");

    createCallThread(conn);
    // printf("Created thread=%lu\n", conn->thread);
    // void* retval;
    // pthread_join(thread, &retval);

    // if ((int) retval == 1) {
    //     printf("[WARN] Call_begin: cannot start call.\n");
    //     return 0;
    // }

    return CALL_START;
}

void Call_terminate(Connection* conn) {
    shutdown(conn->socket, SHUT_RDWR);
    printf("Done TCP socket shutdown.\n");
    // printf("Joining thread=%lu\n", conn->thread);
    // pthread_cancel(conn->thread);
    // pthread_join(conn->thread, NULL);
    Audio_stop();
}

/**
 * Note to future self: don't do anything fancy plz.
 */
static void createCallThread(Connection* conn) {
    // if (numCallThreads == CALL_THREADS_MAX) {
    //     fprintf(stderr, "[FATAL] Maximum number of simultaneous calls (%d) reached.\n", CALL_THREADS_MAX);
    //     exit(1);
    // }

    // int idx = numCallThreads;
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);
    pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
    pthread_create(&conn->thread, &attrs, beginCall, (void*) conn);

    // printf("pthread_create with thread=%lu\n", conn->thread);

    // numCallThreads++;
}

/**
 * Detached thread runner.
 */
static void* beginCall(void* ptr) {
    // InitializePinkNoise(&noise, 12);
    Connection* conn = (Connection*) ptr;

    for (int i = 0; i < MIN_PLAYBACK_QUEUE_LENGTH; i++) {
        send(conn->socket, totalSilence, FRAMEBUFFER_SIZE, 0);
    }

    AudioResult result = Audio_start(receiveDatagram, sendDatagram, conn);

    // We ignore AUDIO_ALREADY_STARTED because you can call yourself.
    // A more sophisticated program would handle this case specifically.
    // In our case, we can assume the only time the Audio module gets
    // started twice is when we call ourselves.
    if (result != AUDIO_OK && result != AUDIO_ALREADY_STARTED) {
        printf("[WARN] beginCall: audio not OK (Audio_start returned %d).\n", result);
        return NULL;
    }

    struct pollfd pollParams = {
        conn->socket, 0, 0
    };

    pollParams.events = POLLRDHUP;
    // size_t bytesAvailable;
    // int ioctlRes;

    // printf("Polling socket...\n");

    // while (1) {
    //     ioctlRes = ioctl(conn->socket, FIONREAD, &bytesAvailable);

    //     if (ioctlRes < 0) {
    //         fprintf(stderr, "[FATAL] ioctl returned -1, socket=%d\n", conn->socket);
    //         perror("ioctl");
    //         exit(1);
    //     }

    //     if (bytesAvailable == 0) {
    //         break;
    //     }

    //     sleep(1);
    // }

    int pollRes = poll(&pollParams, 1, -1);

    if (pollRes < 0) {
        fprintf(stderr, "[FATAL] Poll returned -1\n");
        exit(1);
    }

    printf("\nPoll finished. Exiting callThread.\n");
    // printf("No more bytes to read, call over.\n");

    pthread_exit(NULL);
}

static AudioCallbackResult sendDatagram(FrameBuffer buffer, const size_t bufferSize, void* data) {
    Connection* connection = (Connection*) data;

    // send() will block in non-blocking mode on STREAM_SOCK if only a partial packet is sent.
    ssize_t bytesSent = send(connection->socket, buffer, bufferSize, 0);
    ssize_t totalBytesSent = bytesSent;

    // send() can send out partial frames.
    while (bytesSent < bufferSize) {
        if (bytesSent < 0) {
            // printf("send: AUDIO_STOP\n");
            // shutdown(connection->socket, SHUT_RDWR);
            // setError(errno);
            return AUDIO_STOP;
        }

        bytesSent = send(connection->socket, buffer + totalBytesSent, bufferSize - totalBytesSent, 0);
        totalBytesSent += bytesSent;
    }

    // printf("Sent %ld bytes\n", (long)totalBytesSent);

    return AUDIO_CONTINUE;

}

static AudioCallbackResult receiveDatagram(FrameBuffer buffer, const size_t bufferSize, void* data) {
    Connection* connection = (Connection*) data;
    ssize_t bytesReceived = recv(connection->socket, buffer, bufferSize, 0);
    ssize_t totalBytesReceived = bytesReceived;

    while (totalBytesReceived < bufferSize) {
        if (bytesReceived <= 0) {
            // printf("recv: AUDIO_STOP\n");
            return AUDIO_STOP;
        }

        bytesReceived = recv(connection->socket, buffer + totalBytesReceived, bufferSize - totalBytesReceived, 0);
        totalBytesReceived += bytesReceived;
    }

    // printf("Recv %ld bytes\n", (long)totalBytesReceived);

    return AUDIO_CONTINUE;
}
