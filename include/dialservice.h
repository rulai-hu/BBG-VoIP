#ifndef DIALSERVICE_H
#define DIALSERVICE_H

typedef void (*DialEventHandler)(const char*);

void DialService_start(DialEventHandler);
void DialService_stop(void);

void DialService_suspend(void);
void DialService_resume(void);

#endif
