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

static void receiveDatagram(FrameBuffer, const size_t);
static void sendDatagram(FrameBuffer, const size_t);
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

    pollParams.events = POLLRDHUP | POLLHUP;

    int pollRes = poll(&pollParams, 1, -1);

    return 1;
}

void Call_terminate() {
    Audio_stop();
}

static void sendDatagram(FrameBuffer buffer, const size_t bufferSize) {
    ssize_t bytesSent = send(connection->socket, buffer, bufferSize, 0);

    if (bytesSent < 0) {
        setError(errno);
    }

    // send() can send out partial frames.
    // while (bytesSent < bufferSize) {
    //     bytesSent += send(connection->socket, buffer + bytesSent, bufferSize - bytesSent, 0);
    // }
}

static void receiveDatagram(FrameBuffer buffer, const size_t bufferSize) {
    ssize_t bytesReceived = recv(connection->socket, buffer, bufferSize, 0);


}

static void setError(int error) {

}
