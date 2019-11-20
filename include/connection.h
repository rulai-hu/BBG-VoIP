// This C module manages a socket connection on port 5060 to send/receive data.

#ifndef CONNECTION_H
#define CONNECTION_H

#include <pthread.h>

#include "include/addressbook.h"

#define FileDescriptor int

typedef enum {
    CONNECTION_OK = 0,
    CONNECTION_BUSY,
    CONNECTION_SOCKET_FAIL,
    CONNECTION_CONNECT_FAIL
} ConnectionResult;

// Container for all necessary values of the connection
typedef struct {
    FileDescriptor socket;
    struct sockaddr_in sourceHost;
    pthread_t thread; // Thread of Call using this connection
} Connection;

// This function creates a new connection and returns its status.
ConnectionResult Connection_create(Connection*, Address*);

// This function closes the connection.
void Connection_close(Connection*);

// This function closes the connection and lets the user know the remote end
// hung up.
void Connection_reject(Connection*);

// This function listens for incoming data on the socket.
FileDescriptor Connection_listen();

#endif
