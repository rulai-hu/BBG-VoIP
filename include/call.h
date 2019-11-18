#ifndef CALL_H
#define CALL_H

#include <pthread.h>

#include "include/connection.h"
#include "include/connection.h"

// #define CALL_THREADS_MAX 10

int Call_begin(Connection*);
void Call_terminate(Connection*);

#endif
