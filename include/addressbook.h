#ifndef ADDRESS_H
#define ADDRESS_H

#include <arpa/inet.h>

typedef enum {
    ADDRESS_FOUND = 0, NAME_FOUND, ADDRESS_UNKNOWN, ADDRESS_REVERSE_LOOKUP_FAILED
} AddressLookupResult;

typedef struct {
    const char name[64];
    const char inetAddress[INET_ADDRSTRLEN];
    struct in_addr _in_addr; // this field *must* be filled with the output of inet_pton
} Address;

int AddressBook_lookup(const char*, Address*);
int AddressBook_reverseLookup(const char*, Address*);
void AddressBook_init(void);

#endif
