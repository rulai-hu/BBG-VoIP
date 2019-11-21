#ifndef CALL_H
#define CALL_H

#include <pthread.h>

#include "include/connection.h"
#include "include/connection.h"

// #define CALL_THREADS_MAX 10

typedef enum {
    CALL_START = 0, CALL_FAIL, CALL_ACCEPT, CALL_REJECT
} CallResult;

CallResult Call_begin(Connection*);
void Call_terminate(Connection*);

void Call_accept(Connection*);
void Call_reject(Connection*);

#endif
