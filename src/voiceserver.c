#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "include/voiceserver.h"

static pthread_t voiceServerThread;

static void* listenForConnections(void*);

int stop = 0;

void VoiceServer_start(IncomingCallEventHandler callback) {
    stop = 0;

    int res = pthread_create(&voiceServerThread, NULL, listenForConnections, NULL);

    if (res != 0) {
        printf("VoiceServer: pthread_create failed.\n");
        exit(1);
    }

    printf("VoiceServer started.\n");
}

void VoiceServer_stop() {
    printf("VoiceServer stopped.\n");
}

void* listenForConnections(void* ptr) {
    while (!stop) {

    }

    pthread_exit(NULL);
}
