#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "include/dialservice.h"

#define INPUT_BUFFER_LENGTH 32

static void* getInput(void*);
static void startThread(void);

static pthread_t dialServiceThread;
static pthread_mutex_t inputMutex = PTHREAD_MUTEX_INITIALIZER;

static char inputBuffer[64];
static int stop = 0;
static DialEventHandler dialEventHandler;

void DialService_start(DialEventHandler callback) {
    stop = 0;
    dialEventHandler = callback;

    startThread();

    printf("DialService started.\n");
}

void DialService_suspend() {
    pthread_cancel(dialServiceThread);
    printf("DialService suspended.\n");
}

void DialService_resume() {
    startThread();
    printf("DialService resumed.\n");
}

void DialService_stop() {
    // stop = 1;

    int res = pthread_join(dialServiceThread, NULL);
    pthread_mutex_destroy(&inputMutex);

    if (res != 0) {
        printf("Unable to join dialServiceThread.\n");
    }

    printf("DialService stopped.\n");
}

static void startThread() {
    int res = pthread_create(&dialServiceThread, NULL, getInput, NULL);

    if (res != 0) {
        printf("DialService: pthread_create failed.\n");
        exit(1);
    }
}

static void* getInput(void* ptr) {
    char localBuffer[INPUT_BUFFER_LENGTH];

    while (!stop) {
        memset(localBuffer, '\0', sizeof(localBuffer));

        printf("Call someone: ");

        // fgets is a cancellation point for pthread_cancel
        fgets(localBuffer, sizeof(localBuffer), stdin);

        // clear stdin
        if (strlen(localBuffer) >= (INPUT_BUFFER_LENGTH - 1)) {
            int c;
            while ((c = fgetc(stdin)) != EOF && c != '\n');
        }

        // remove trailing newline from inputBuffer
        char* pos;
        if ((pos = strchr(localBuffer, '\n')) != NULL) {
            *pos = '\0';
        }

        // pthread_mutex_lock(&inputMutex);
        memcpy(inputBuffer, localBuffer, sizeof(inputBuffer));
        dialEventHandler(inputBuffer);
        // pthread_mutex_unlock(&inputMutex);
    }

    pthread_exit(NULL);
}
