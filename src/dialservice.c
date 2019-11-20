#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "include/dialservice.h"

#define INPUT_BUFFER_LENGTH 32

static void* getInput(void*);
static void startThread(void);

static pthread_t dialServiceThread;
static pthread_mutex_t callMutex = PTHREAD_MUTEX_INITIALIZER;

static char inputBuffer[INPUT_BUFFER_LENGTH];
static int serviceSuspended;
static int stop = 0;
static DialEventHandler dialEventHandler;

void DialService_start(DialEventHandler callback) {
    stop = 0;
    serviceSuspended = 0;
    dialEventHandler = callback;

    startThread();

    printf("DialService started.\n");
}

void DialService_suspend() {
    if (serviceSuspended) {
        return;
    }

    serviceSuspended = 1;

    // Attempt to cancel thread
    pthread_mutex_lock(&callMutex);
    int res = pthread_cancel(dialServiceThread);
    pthread_mutex_unlock(&callMutex);

    if (res != 0) {
        fprintf(
            stderr,
            "[WARN] DialService_suspend: " \
            "pthread_cancel failed, thread doesn't exist.\n"
        );
    }

    void* exitResult;
    pthread_join(dialServiceThread, &exitResult);

    if (exitResult == PTHREAD_CANCELED) {
        printf("[INFO] DialService_suspend: DialService suspended.\n");
    } else {
        fprintf(
            stderr,
            "[WARN] DialService_suspend: unable to suspend DialService..\n"
        );
    }
}

void DialService_resume() {
    if (!serviceSuspended) {
        return;
    }

    // Consume stdin before starting service.
    int ch;
    while ((ch = fgetc(stdin)) != EOF && ch != '\n') {}
    serviceSuspended = 0;
    startThread();
    printf("[INFO] DialService resumed.\n");
}

void DialService_stop() {
    stop = 1;

    DialService_suspend();

    // void* exitResult;
    // int res = pthread_join(dialServiceThread, &exitResult);

    pthread_mutex_destroy(&callMutex);

    // if (res != 0 && exitResult != PTHREAD_CANCELED) {
    //     fprintf(stderr, "Unable to join dialServiceThread.\n");
    // }

    printf("DialService stopped.\n");
}

static void startThread() {
    int res = pthread_create(&dialServiceThread, NULL, getInput, NULL);

    if (res != 0) {
        fprintf(stderr, "DialService: pthread_create failed.\n");
        exit(1);
    }
}

static void* getInput(void* ptr) {
    char localBuffer[INPUT_BUFFER_LENGTH];

    while (!stop && !serviceSuspended) {
        memset(localBuffer, '\0', sizeof(localBuffer));

        printf("Call someone: ");

        // fgets is a cancellation point for pthread_cancel
        if (fgets(localBuffer, sizeof(localBuffer), stdin) == NULL) {
            fprintf(stderr, "An error occurred while reading input. Try again.\n");
            continue;
        }

        // Clear stdin
        if (strlen(localBuffer) >= (INPUT_BUFFER_LENGTH - 1)) {
            int ch;
            while ((ch = fgetc(stdin)) != EOF && ch != '\n') {}

            // if (ch == '\n') {
            //     ungetc(ch, stdin);
            // }
        }

        // Remove trailing newline from inputBuffer
        char* pos;
        if ((pos = strchr(localBuffer, '\n')) != NULL) {
            *pos = '\0';
        }

        // Run Event Handler method with completed dial input
        pthread_mutex_lock(&callMutex);
        memcpy(inputBuffer, localBuffer, sizeof(inputBuffer));
        dialEventHandler(inputBuffer);
        pthread_mutex_unlock(&callMutex);
    }

    pthread_exit(NULL);
}
