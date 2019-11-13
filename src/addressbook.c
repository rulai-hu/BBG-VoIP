#include <stdlib.h>
#include <arpa/inet.h>

#include "include/addressbook.h"

#define TABLE_SIZE 1

static Address lookupTable[TABLE_SIZE] = {
    {"localhost", "127.0.0.1"}
};

int AddressBook_lookup(char* name, Address* address) {
    return 1;
}

void AddressBook_init() {
    for (int i = 0; i < TABLE_SIZE; i++) {
        // inet_pton(AF_INET, lookupTable[i].inetAddress);
    }
}
