#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "include/addressbook.h"

static Address lookupTable[] = {
    {"localhost", "127.0.0.1"}
};

#define TABLE_SIZE (sizeof(lookupTable) / sizeof(Address))

int AddressBook_lookup(const char* name, Address* address) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        if (strcmp(lookupTable[i].name, name) == 0) {
            memcpy(address, &lookupTable[i], sizeof(Address));
            return ADDRESS_FOUND;
        }
    }

    return ADDRESS_UNKNOWN;
}

int AddressBook_reverseLookup(const char* ip, Address* address) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        if (strcmp(lookupTable[i].inetAddress, ip) == 0) {
            memcpy(address, &lookupTable[i], sizeof(Address));
            return NAME_FOUND;
        }
    }

    return ADDRESS_REVERSE_LOOKUP_FAILED;
}

void AddressBook_init() {
    for (int i = 0; i < TABLE_SIZE; i++) {
        int res = inet_pton(AF_INET, lookupTable[i].inetAddress, &(lookupTable[i]._in_addr));

        if (res != 1) {
            fprintf(stderr, "AddressBook_init failed because inet_pton failed. Ensure that entries are correctly formed.\n");
            exit(1);
        }
    }
}
