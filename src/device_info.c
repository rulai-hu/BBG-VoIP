#include <stdio.h>
#include "include/portaudio.h"

int main(void) {
    // Required before calling any Pa_* functions.
    Pa_Initialize();

    int n = Pa_GetDeviceCount();
    printf("Number of devices found: %d\n", n);

    if (n < 0) {
        printf( "[ERROR] Pa_GetDeviceCount returned error code %d\n", n);
        return 1;
    }

    const PaDeviceInfo *deviceInfo;

    for (int i = 0; i < n; i++) {
        deviceInfo = Pa_GetDeviceInfo(i);
        printf("Device #%d: name=%s\n", i, deviceInfo->name);
    }

    Pa_Terminate();
}
