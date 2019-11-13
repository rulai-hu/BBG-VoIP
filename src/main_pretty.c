#include <signal.h>
#include <stdio.h>

#include "include/voiceserver.h"
#include "include/dialservice.h"
#include "include/addressbook.h"
#include "include/connection.h"
#include "include/call.h"

#define CALLEE "localhost"

static void onDial(char*);
static void handleIncomingCall(Address*, Connection*);

void SIGINTHandler(int sig) {
    signal(SIGINT, SIGINTHandler);
    DialService_suspend();
    fflush(stdout);
}

int main(void) {
    AddressBook_init();
    // signal(SIGINT, SIGINTHandler);
    VoiceServer_start(handleIncomingCall);
    DialService_start(onDial);

    DialService_stop();
    VoiceServer_stop();

    return 0;
}

static void onDial(char* name) {
    Address dest;
    AddressBookLookupResult lookupResult = AddressBook_lookup(name, &dest);

    if (lookupResult != ADDRESS_FOUND) {
        printf("Address not found for %s\n", name);
        return;
    }

    printf("Found host. Establishing connection to %s...", dest.inetAddress);

    Connection connection;
    ConnectionResult connResult = Connection_create(&connection, &dest);

    if (connResult != CONNECTION_OK) {
        printf("Unable to connect.\n");
        return;
    }

    Call_begin(&connection);

    // poll(connection->closed, blah blah)

    Call_terminate();
    Connection_close();
}

static void handleIncomingCall(Address* caller, Connection* conn) {
    // Prevent user from dialing out during call.
    DialService_suspend();
    printf("Incoming call from %s\n", caller->name);
    Call_begin(conn);

    // poll(....)

    Call_terminate();
    Connection_close();

    DialService_resume();
}
