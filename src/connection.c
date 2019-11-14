#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "include/addressbook.h"
#include "include/connection.h"

#define PORT 5060

int Connection_create(Connection* conn, Address* addr) {
    conn->socket = socket(AF_INET, SOCK_STREAM, 0);

    if (conn->socket < 0) {
        perror("Connection_create");
        return CONNECTION_SOCKET_FAIL;
    }

    memset(&conn->sourceHost, 0, sizeof(conn->sourceHost));
    conn->sourceHost.sin_family = AF_INET;
    conn->sourceHost.sin_port = htons(PORT);
    conn->sourceHost.sin_addr = addr->_in_addr;

    int res = connect(conn->socket, (struct sockaddr *) &conn->sourceHost, sizeof(conn->sourceHost));

    // Should handle EISCONN, ENETUNREACH, ECONNREFUSED cases separately
    // instead of bundling them together into an unhelpful error number.
    if (res < 0) {
        perror("Connection_create");
        return CONNECTION_CONNECT_FAIL;
    }

    return CONNECTION_OK;
}

void Connection_close() {

}

void Connection_reject(Connection* conn) {
    close(conn->socket);
}

FileDescriptor Connection_listen() {
    FileDescriptor sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("Connection_listen");
        exit(1);
    }

    int res;

    // char buffer[1024] = {0};
    // char *hello = "Hello from server";

    // int option = 1;
    // res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option));

    // if (res < 0) {
    //     perror("setsockopt");
    //     exit(1);
    // }

    struct sockaddr_in hostAddr;

    hostAddr.sin_family = AF_INET;
    hostAddr.sin_addr.s_addr = INADDR_ANY;
    hostAddr.sin_port = htons(5060);

    res = bind(sock, (struct sockaddr *) &hostAddr, sizeof(hostAddr));

    if (res < 0) {
        perror("bind failed");
        exit(1);
    }

    // no backlog
    res = listen(sock, 0);

    if (res < 0) {
        perror("listen");
        exit(1);
    }

    return sock;

    // int addrlen;
    // int tcpSocket = accept(sock, (struct sockaddr *) &peerAddr, (socklen_t*) &addrlen);

    // if (tcpSocket <0) {
    //     perror("accept");
    //     exit(1);
    // }

    // valread = read( new_socket , buffer, 1024);
    // printf("%s\n",buffer );
    // send(new_socket , hello , strlen(hello) , 0 );
    // printf("Hello message sent\n");
    // return 0;

}
