// This C module manages a new thread which allows calling another user.

#ifndef DIALSERVICE_H
#define DIALSERVICE_H

// Type definition for the Event Handler function to be passed.
typedef void (*DialEventHandler)(const char*);

// This function spins up a new thread and starts the Dial Service there.
// Once a suitable input is received, the DialEventHandler function will be
// evoked on the input.
// Must be called before any of the other following methods.
// 
// DialEventHandler should not be null.
void DialService_start(DialEventHandler);

// This function shuts down the running Dial Service thread from start().
// If the service is not currently running, an error will be sent.
void DialService_stop(void);

// This function removes the Dial Service thread and suspends operation.
// If the service is already suspended, nothing will happen.
// If the service is not currently running, an error will be sent.
void DialService_suspend(void);

// This function restarts the Dial Service thread and continues operation.
// If the service is already running, nothing will happen.
// If the service is not currently running, an error will be sent.
void DialService_resume(void);

#endif
