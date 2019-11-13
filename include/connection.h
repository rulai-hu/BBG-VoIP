#ifndef CONNECTION_H
#define CONNECTION_H

#include "include/address.h"

typedef struct {

} Connection;

int Connection_create(Connection*, Address*);
void Connection_close(void);

#endif
