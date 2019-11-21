#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <poll.h>

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

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Please provide the input audio device index, then output device index.\n");
        return 1;
    }

    int inputDeviceIndex = atoi(argv[1]);
    int outputDeviceIndex = atoi(argv[2]);

    printf("Audio input will be bound to device %d and output to device %d. Press Ctrl-C to quit.\n", inputDeviceIndex, outputDeviceIndex);

    // Signal handling stuff...
    sigset_t signalSet;
    int signalNumber;
    sigemptyset(&signalSet);
    sigaddset(&signalSet, SIGINT);
    pthread_sigmask(SIG_BLOCK, &signalSet, NULL);

    Audio_init(inputDeviceIndex, outputDeviceIndex);

    AddressBook_init();
    VoiceServer_start(handleIncomingCall);
    DialService_start(onDial);

    // Wait for SIGINT to arrive.
    sigwait(&signalSet, &signalNumber);

    printf("\nSIGINT received by main thread. Stopping program...\n");

    DialService_stop();
    VoiceServer_stop();
    Audio_stop();
    Audio_teardown();

    printf("Bye.\n");

    return 0;
}

static void onDial(const char* name) {
    Address dest;
    AddressLookupResult lookupResult = AddressBook_lookup(name, &dest);

    if (lookupResult != ADDRESS_FOUND) {
        printf("Address not found for '%s'.\n", name);
        return;
    }

    printf("Establishing connection to %s (%d)\n", dest.inetAddress, dest._in_addr.s_addr);

    Connection connection;
    ConnectionResult connResult = Connection_create(&connection, &dest);

    if (connResult != CONNECTION_OK) {
        fprintf(stderr, "Unable to connect.\n");
        return;
    }

    printf("Connection established through socket %d.\n", connection.socket);

    // Extend our end of the handshake.
    Call_accept(&connection);

    CallResult callResult = Call_begin(&connection);

    printf("Call has started. Press 'Ctrl-D' to hang up.\n");

    if (callResult == CALL_FAIL) {
        fprintf(stderr, "Unable to start call with %s.\n", name);
        return;
    }

    char ch;

    while (((read(0, &ch, 1) == 1) ? (unsigned char) ch : EOF) != EOF) {
        printf("Unrecognized command.\n");
    }

    Call_terminate(&connection);
    Connection_close(&connection);
}

static void handleIncomingCall(Address* caller, Connection* conn) {
    // Prevent user from dialing out during call.
    DialService_suspend();
    printf("Incoming call from %s. Accept? [y/n] ", caller->name);

    while (1) {
        char ch = fgetc(stdin);

        if (ch == 'y' || ch == 'Y') {
            Call_accept(conn);
            break;
        } else if (ch == 'n' || ch == 'N') {
            Call_reject(conn);
            DialService_resume();
            return;
        }

        fprintf(stderr, "Not a valid input. Try again.\n");
    }

    // printf("Call is on socket %d.\n", conn->socket);

    Call_begin(conn);
    printf("Call has started. Press 'Ctrl-D' to hang up.\n");

    char ch;

    while (((read(0, &ch, 1) == 1) ? (unsigned char) ch : EOF) != EOF) {
        printf("Unrecognized command.\n");
    }

    Call_terminate(conn);
    Connection_close(conn);

    DialService_resume();
}
