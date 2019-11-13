#ifndef CONNECTION_H
#define CONNECTION_H

#include "include/addressbook.h"

typedef enum {
    CONNECTION_OK = 0, CONNECTION_BUSY
} ConnectionResult;

typedef struct {

} Connection;

int Connection_create(Connection*, Address*);
void Connection_close(void);

#endif
