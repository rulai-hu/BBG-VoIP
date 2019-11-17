#include <signal.h>
#include <stdio.h>

#include "include/voiceserver.h"
#include "include/dialservice.h"
#include "include/addressbook.h"
#include "include/connection.h"
#include "include/audio.h"
#include "include/call.h"

static void onDial(const char*);
static void handleIncomingCall(Address*, Connection*);

// void SIGINTHandler(int sig) {
//     signal(SIGINT, SIGINTHandler);
//     DialService_suspend();
//     fflush(stdout);
// }

int main(void) {
    // Signal handling stuff...
    sigset_t signalSet;
    int signalNumber;
    sigemptyset(&signalSet);
    sigaddset(&signalSet, SIGINT);
    pthread_sigmask(SIG_BLOCK, &signalSet, NULL);

    // signal(SIGINT, SIGINTHandler);
    Audio_init();
    AddressBook_init();
    VoiceServer_start(handleIncomingCall);
    DialService_start(onDial);

    // Wait for SIGINT to arrive.
    sigwait(&signalSet, &signalNumber);

    printf("\nSIGINT received by main thread. Stopping program...\n");

    DialService_stop();
    VoiceServer_stop();
    Audio_teardown();

    printf("Bye.\n");
    fflush(stdout);

    return 0;
}

static void onDial(const char* name) {
    Address dest;
    AddressLookupResult lookupResult = AddressBook_lookup(name, &dest);

    if (lookupResult != ADDRESS_FOUND) {
        printf("Address not found for '%s'.\n", name);
        return;
    }

    printf("Found host. Establishing connection to %s (%d)\n", dest.inetAddress, dest._in_addr.s_addr);

    Connection connection;
    ConnectionResult connResult = Connection_create(&connection, &dest);

    if (connResult != CONNECTION_OK) {
        fprintf(stderr, "Unable to connect.\n");
        return;
    }

    printf("Connection established through socket %d.\n", connection.socket);

    DialService_suspend();

    int callResult = Call_begin(&connection);

    if (callResult == 0) {
        fprintf(stderr, "Unable to start call with %s.\n", name);
        return;
    }

    // poll(connection->closed, blah blah)

    Call_terminate(&connection);
    Connection_close(&connection);

    DialService_resume();
}

static void handleIncomingCall(Address* caller, Connection* conn) {
    // Prevent user from dialing out during call.
    DialService_suspend();
    printf("Incoming call from %s. Accept? [y/n] ", caller->name);

    while (1) {
        char ch = fgetc(stdin);

        if (ch == 'y' || ch == 'Y') {
            break;
        } else if (ch == 'n' || ch == 'N') {
            Connection_reject(conn);
            DialService_resume();
            return;
        }

        fprintf(stderr, "Not a valid input. Try again.\n");
    }

    // printf("Call is on socket %d.\n", conn->socket);

    Call_begin(conn);

    // poll(....)

    Call_terminate(conn);
    Connection_close();

    DialService_resume();
}
