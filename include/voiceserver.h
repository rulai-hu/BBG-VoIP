#ifndef VOICESERVER_H
#define VOICESERVER_H

#include "include/addressbook.h"
#include "include/connection.h"

typedef void (*IncomingCallEventHandler)(Address*, Connection*);

void VoiceServer_start(IncomingCallEventHandler);
void VoiceServer_stop(void);

#endif
