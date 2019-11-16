// This is non-portable, only works on Linux 2.6+
// We need for poll event POLLRDHUP
#define _GNU_SOURCE

#ifdef __APPLE__
// Get rid of linter errors for developing in a Mac environment
#define POLLRDHUP 0x0000
#endif

#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <poll.h>

#include "include/connection.h"
#include "include/audio.h"
#include "include/call.h"

static AudioCallbackResult receiveDatagram(FrameBuffer, const size_t);
static AudioCallbackResult sendDatagram(FrameBuffer, const size_t);
static void setError(int);

const Connection* connection;

int Call_begin(const Connection* conn) {
    connection = conn;

    AudioResult res = Audio_start(receiveDatagram, sendDatagram);

    if (res != AUDIO_OK) {
        return 0;
    }

    struct pollfd pollParams = {
        conn->socket, 0, 0
    };

    pollParams.events = POLLRDHUP;

    int pollRes = poll(&pollParams, 1, -1);

    return 1;
}

void Call_terminate() {
    Audio_stop();
}

static AudioCallbackResult sendDatagram(FrameBuffer buffer, const size_t bufferSize) {
    // send will block in non-blocking mode on STREAM_SOCK if only a partial packet is sent.
    ssize_t bytesSent = send(connection->socket, buffer, bufferSize, 0);

    if (bytesSent < 0) {
        shutdown(connection->socket, SHUT_RDWR);
        setError(errno);
        return AUDIO_STOP_RECORDING;
    }

    return AUDIO_CONTINUE_RECORDING;

    // send() can send out partial frames.
    // while (bytesSent < bufferSize) {
    //     bytesSent += send(connection->socket, buffer + bytesSent, bufferSize - bytesSent, 0);
    // }
}

static AudioCallbackResult receiveDatagram(FrameBuffer buffer, const size_t bufferSize) {
    ssize_t bytesReceived = recv(connection->socket, buffer, bufferSize, 0);
    ssize_t totalBytesReceived = bytesReceived;

    while (totalBytesReceived < bufferSize) {
        if (bytesReceived <= 0) {
            return AUDIO_STOP_PLAYBACK;
        }

        bytesReceived = recv(connection->socket, buffer + totalBytesReceived, bufferSize - totalBytesReceived, 0);
        totalBytesReceived += bytesReceived;
    }


    return AUDIO_CONTINUE_PLAYBACK;
}

static void setError(int error) {

}
