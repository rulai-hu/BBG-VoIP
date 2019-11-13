#include <stdio.h>

#include "include/address.h"
#include "include/connection.h"
#include "include/call.h"

#define CALLEE "localhost"

int main(void) {
    Address destAddress;
    int lookupResult = Address_lookup(CALLEE, &destAddress);

    if (lookupResult != 0) {
        printf("Address not found.\n");
        return 1;
    }

    printf("Found host. Establishing connection to %s...", CALLEE);

    Connection connection;
    int connResult = Connection_create(&connection, &destAddress);

    if (connResult != 0) {
        printf("Unable to connect.\n");
        return 1;
    }

    Call_begin(&connection);

    Call_terminate();
    Connection_close();

    return 0;
}
