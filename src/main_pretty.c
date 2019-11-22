// This is non-portable, only works on Linux 2.6+
// We need for poll event POLLRDHUP
#define _GNU_SOURCE

#ifdef __APPLE__
// Get rid of linter errors for developing in a Mac environment
#define POLLRDHUP 0x0000
#endif

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
#include "../include/keypad.h"
#include "../include/led.h"

static void onDial(const char*);
static void handleIncomingCall(Address*, Connection*);

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Please provide the input audio device index, then output device index.\n");
        return 1;
    }

    int inputDeviceIndex = atoi(argv[1]);
    int outputDeviceIndex = atoi(argv[2]);

    printf("Audio input bound to device %d and output to device %d.\nPress Ctrl-C to quit.\n", inputDeviceIndex, outputDeviceIndex);

    // Signal handling stuff...
    sigset_t signalSet;
    int signalNumber;
    sigemptyset(&signalSet);
    sigaddset(&signalSet, SIGINT);
    pthread_sigmask(SIG_BLOCK, &signalSet, NULL);

    KEYPAD_init();
    Audio_init(inputDeviceIndex, outputDeviceIndex);

    LED_init();
    AddressBook_init();
    VoiceServer_start(handleIncomingCall);
    DialService_start(onDial);

    // Wait for SIGINT to arrive.
    sigwait(&signalSet, &signalNumber);

    printf("\nSIGINT received by main thread. Stopping program...\n");

    DialService_stop();
    VoiceServer_stop();
    printf("Audio_stop TEARDOWN\n");
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

    LED_blu_on();

    struct pollfd fds[2];

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = connection.socket;
    fds[1].events = POLLRDHUP;

    int ch;
    int ret;

    while (1) {
        ret = poll(fds, 2, -1);
        if (fds[0].revents & POLLIN) {
            ch = fgetc(stdin);
            if (ch == EOF) {
                break;
            } else {
                printf("Unrecognized command.\n");
            }
        }

        if (fds[1].revents & POLLRDHUP) {
            printf("POLLRDHUP!\n");
            break;
        }
    }


    // while (ch = fgetc(stdin), ch != EOF) {
    //     printf("Unrecognized command.\n");
    // }

    LED_blu_off();
    printf("Terminating outbound call...\n");
    Call_terminate(&connection);
    printf("Done terminating..\n");
    // Connection_close(&connection);
}

static void handleIncomingCall(Address* caller, Connection* conn) {
    // Prevent user from dialing out during call.
    DialService_suspend();
    printf("Incoming call from %s. Accept? [y/n] ", caller->name);

    while (1) {
        char ch = fgetc(stdin);

        if (ch == 'y' || ch == 'Y') {
            Call_accept(conn);
            LED_blu_on();
            break;
        } else if (ch == 'n' || ch == 'N') {
            Call_reject(conn);
            DialService_resume();
            return;
        }

        fprintf(stderr, "Not a valid input. Try again.\n");
    }

    int ch;
    // Consume stdin
    while ((ch = fgetc(stdin)) != EOF && ch != '\n') {}

    // printf("Call is on socket %d.\n", conn->socket);

    Call_begin(conn);
    printf("Call has started. Press 'Ctrl-D' to hang up.\n");

    struct pollfd fds[2];

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = conn->socket;
    fds[1].events = POLLRDHUP;

    int ret;

    while (1) {
        ret = poll(fds, 2, -1);
        if (fds[0].revents & POLLIN) {
            ch = fgetc(stdin);
            if (ch == EOF) {
                break;
            } else {
                printf("Unrecognized command.\n");
            }
        }

        if (fds[1].revents & POLLRDHUP) {
            printf("POLLRDHUP!\n");
            break;
        }
    }

    // if (ret == -1) {
    //     perror("poll");
    //     return;
    // }

    // if (fds[0].revents & POLLIN)
    //     printf("fd:0 POLLIN\n");

    // if (fds[1].revents & POLLRDHUP)
    //     printf("fd:1 POLLRDHUP\n");

    // while (ch = fgetc(stdin), ch != EOF) {
    //     printf("Unrecognized command.\n");
    // }

    LED_red_off();
    LED_blu_off();

    printf("Terminating inbound call\n");
    Call_terminate(conn);
    printf("Done terminating inbound\n");
    // Connection_close(conn);

    DialService_resume();
}
