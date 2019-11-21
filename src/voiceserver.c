#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "include/connection.h"
#include "include/addressbook.h"

#include "include/voiceserver.h"

static pthread_t voiceServerThread;

static void* acceptConnections(void*);

static int listenSocket;
static int stop = 0;
static IncomingCallEventHandler handleIncomingCall;

void VoiceServer_start(IncomingCallEventHandler callback) {
    stop = 0;

    handleIncomingCall = callback;

    listenSocket = Connection_listen();

    // Start the server in a new thread
    int res = pthread_create(&voiceServerThread, NULL, acceptConnections, NULL);
    if (res != 0) {
        printf("VoiceServer: pthread_create failed.\n");
        exit(1);
    }
    printf("VoiceServer: Started.\n");
}

void VoiceServer_stop() {
    stop = 1;

    // Stop and rejoin the threads
    int res = pthread_cancel(voiceServerThread);
    if (res != 0) {
        fprintf(stderr, "pthread_cancel failed: Thread may not exist.\n");
    }

    void* exitResult;
    pthread_join(voiceServerThread, &exitResult);

    close(listenSocket);

    if (exitResult == PTHREAD_CANCELED) {
        printf("VoiceServer: Stopped.\n");
    } else {
        fprintf(stderr, "Unable to stop VoiceServer.\n");
    }
}

static void* acceptConnections(void* ptr) {
    int tcpSocket;
    struct sockaddr_in peerAddr;

    // This must be set to the size of the input struct.
    int peerAddressSize = sizeof(peerAddr);
    char callerIp[INET_ADDRSTRLEN];
    Connection connection;
    Address callerAddr;

    while (!stop) {
        // Clear client address and set the socket
        memset(&peerAddr, 0, sizeof(struct sockaddr_in));
        tcpSocket = accept(
            listenSocket,
            (struct sockaddr*) &peerAddr,
            (socklen_t*) &peerAddressSize
        );

        // Set IP address
        const char* ptr = inet_ntop(
            AF_INET,
            &peerAddr.sin_addr.s_addr,
            callerIp,
            INET_ADDRSTRLEN
        );
        // printf(
        //     "ListenSocket=%d, TCPSocket=%d, s_addr=%d, CallerIP=%s\n",
        //     listenSocket, tcpSocket, peerAddr.sin_addr.s_addr, callerIp
        // );
        if (ptr == NULL) {
            perror("This shouldn't happen!");
            exit(1);
        }

        // Find and connect with the caller
        connection.socket = tcpSocket;
        memcpy(&connection.sourceHost, &peerAddr, peerAddressSize);
        int res = AddressBook_reverseLookup(callerIp, &callerAddr);

        if (res == ADDRESS_REVERSE_LOOKUP_FAILED) {
            printf("VoiceServer: Unknown caller "\
              "(possibly NSA). Closing connection...\n");
            close(tcpSocket);
            continue;
        }

        // printf("VoiceServer: Handling incoming call...\n");
        handleIncomingCall(&callerAddr, &connection);
    }

    return NULL;
}
