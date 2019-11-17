// This C module provides functions to start and stop a VoiceServer
// which can transit and receive audio.

#ifndef VOICESERVER_H
#define VOICESERVER_H

#include "include/addressbook.h"
#include "include/connection.h"

// Pointer to a function which takes Address* and Connection* parameters
typedef void (*IncomingCallEventHandler)(Address*, Connection*);

// This function starts up a new server and initializes the connection.
// A new thread is created for the server to run on.
// Exits if the thread cannot be created.
void VoiceServer_start(IncomingCallEventHandler);

// This function cleans up and sends a request to stop the server.
// If no server is currently running, an error message will be displayed.
void VoiceServer_stop(void);

#endif
