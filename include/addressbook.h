// This C module provides functions to find the caller, given a caller address.

#ifndef ADDRESS_H
#define ADDRESS_H

#include <arpa/inet.h>

typedef enum {
    ADDRESS_FOUND = 0,
    NAME_FOUND,
    ADDRESS_UNKNOWN,
    ADDRESS_REVERSE_LOOKUP_FAILED
} AddressLookupResult;

// Struct to contain all necessary values of the caller's address
typedef struct {
    const char name[64];
    const char inetAddress[INET_ADDRSTRLEN];
    struct in_addr _in_addr; // This field *must* be the output of inet_pton
} Address;

// This function initializes the addresses of the AddressBook to be ready
// for lookup or reverse lookup.
// Exits immediately if the lookup table is configured incorrectly.
void AddressBook_init(void);

// This function returns whether or not a caller name exists in the table.
AddressLookupResult AddressBook_lookup(const char*, Address*);

// This function returns whether or not an IP exists in the table.
AddressLookupResult AddressBook_reverseLookup(const char*, Address*);

#endif
