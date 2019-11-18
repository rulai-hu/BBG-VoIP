#include <sys/socket.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "include/addressbook.h"
#include "include/audio.h"
#include "include/call.h"

static pthread_t tcpListenThread;

// Stub implementation for a voice server
static void* acceptConnections(void*);

int listenSocket;
Address dest;
Connection callout;
Connection peer;

int main(void) {
    Audio_init();
    AddressBook_init();

    // Create the socket where the program will listen for inbound
    // TCP connections.
    listenSocket = Connection_listen();

    pthread_create(&tcpListenThread, NULL, acceptConnections, NULL);

    sleep(1);

    // Now set up everything we need to call ourselves (localhost).
    AddressBook_lookup("localhost", &dest);

    printf("Establishing outbound connection.\n");
    ConnectionResult connResult = Connection_create(&callout, &dest);
    printf("Outbound connection success on socket=%d.\n", callout.socket);

    if (connResult != CONNECTION_OK) {
        fprintf(stderr, "Outbound connection failed.\n");
        return 1;
    }

    printf("Starting call. Hanging up after 5 seconds.\n");
    int callResult = Call_begin(&callout);

    if (callResult == 0) {
        fprintf(stderr, "Unable to start call.\n");
        return 1;
    }

    pthread_join(tcpListenThread, NULL);

    for (int i = 0; i < 5; i++) {
        printf("%d ", i + 1);
        fflush(stdout);
        sleep(1);
    }

    printf("\n");

    printf("Wake up. Terminating outbound call...\n");
    Call_terminate(&callout);
    printf("Terminated outbound. Closing connection... ");
    Connection_close(&callout);
    printf("done.\n");

    Call_terminate(&peer);
    printf("Finished call with inbound. Closing connection... ");
    Connection_close(&peer);
    printf("done.\n");

    Audio_teardown();
    return 0;
}

static void* acceptConnections(void* ptr) {
    printf("Start listening for inbound connections...\n");
    int tcpSocket;
    struct sockaddr_in peerAddr;

    // This must be set to the size of the input struct.
    int peerAddressSize = sizeof(peerAddr);
    Connection connection;

    memset(&peerAddr, 0, sizeof(struct sockaddr_in));
    tcpSocket = accept(listenSocket, (struct sockaddr*) &peerAddr, (socklen_t*) &peerAddressSize);
    // We're connected, so stop listening.
    printf("Inbound connection accepted. Stop listening for new connections.\n");
    close(listenSocket);

    connection.socket = tcpSocket;
    memcpy(&connection.sourceHost, &peerAddr, peerAddressSize);

    printf("Starting call with inbound on socket=%d.\n", tcpSocket);
    memcpy(&peer, &connection, sizeof(Connection));
    Call_begin(&peer);

    return NULL;
}
