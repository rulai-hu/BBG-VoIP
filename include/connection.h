#ifndef CONNECTION_H
#define CONNECTION_H

#include "include/addressbook.h"

#define FileDescriptor int

typedef enum {
    CONNECTION_OK = 0, CONNECTION_BUSY, CONNECTION_SOCKET_FAIL, CONNECTION_CONNECT_FAIL
} ConnectionResult;

typedef struct {
    FileDescriptor socket;
    struct sockaddr_in sourceHost;
} Connection;

int Connection_create(Connection*, Address*);
void Connection_close(void);
void Connection_reject(Connection*);
FileDescriptor Connection_listen();

#endif
