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
// #include <errno.h>
#include <poll.h>

#include "include/connection.h"
#include "include/audio.h"
#include "include/call.h"

static AudioCallbackResult receiveDatagram(FrameBuffer, const size_t, void*);
static AudioCallbackResult sendDatagram(FrameBuffer, const size_t, void*);
// static void setError(int);
static void createCallThread(Connection*);
static void* beginCall(void*);

int Call_begin(Connection* conn) {
    createCallThread(conn);
    // printf("Created thread=%lu\n", conn->thread);
    // void* retval;
    // pthread_join(thread, &retval);

    // if ((int) retval == 1) {
    //     printf("[WARN] Call_begin: cannot start call.\n");
    //     return 0;
    // }

    return 1;
}

void Call_terminate(Connection* conn) {
    // close(conn->socket);
    shutdown(conn->socket, SHUT_RDWR);
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
    Connection* conn = (Connection*) ptr;

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

    // printf("Poll finished. Exiting callThread.\n");
    // printf("No more bytes to read, call over.\n");

    pthread_exit(NULL);
}

static AudioCallbackResult sendDatagram(FrameBuffer buffer, const size_t bufferSize, void* data) {
    Connection* connection = (Connection*) data;

    // send() will block in non-blocking mode on STREAM_SOCK if only a partial packet is sent.
    ssize_t bytesSent = send(connection->socket, buffer, bufferSize, 0);

    if (bytesSent < 0) {
        shutdown(connection->socket, SHUT_RDWR);
        // setError(errno);
        return AUDIO_STOP_RECORDING;
    }

    printf("Send\n");

    return AUDIO_CONTINUE_RECORDING;

    // send() can send out partial frames.
    // while (bytesSent < bufferSize) {
    //     bytesSent += send(connection->socket, buffer + bytesSent, bufferSize - bytesSent, 0);
    // }
}

static AudioCallbackResult receiveDatagram(FrameBuffer buffer, const size_t bufferSize, void* data) {
    Connection* connection = (Connection*) data;
    ssize_t bytesReceived = recv(connection->socket, buffer, bufferSize, 0);
    ssize_t totalBytesReceived = bytesReceived;

    while (totalBytesReceived < bufferSize) {
        if (bytesReceived <= 0) {
            return AUDIO_STOP_PLAYBACK;
        }

        bytesReceived = recv(connection->socket, buffer + totalBytesReceived, bufferSize - totalBytesReceived, 0);
        totalBytesReceived += bytesReceived;
    }

    printf("Recv\n");

    return AUDIO_CONTINUE_PLAYBACK;
}
