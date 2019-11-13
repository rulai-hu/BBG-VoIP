#ifndef ADDRESS_H
#define ADDRESS_H

#include <arpa/inet.h>

typedef enum {
    ADDRESS_FOUND = 0, ADDRESS_UNKNOWN
} AddressBookLookupResult;

typedef struct {
    char name[64];
    char inetAddress[INET_ADDRSTRLEN]; // this _must_ be filled with the output of inet_pton
} Address;

int AddressBook_lookup(char*, Address*);
void AddressBook_init(void);

#endif
