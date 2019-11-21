#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "include/audio.h"
#include "include/pink.h"

static PinkNoise noise;

static AudioCallbackResult consumer(FrameBuffer buf, const size_t sz, void* callData) {
    Sample max = 0;
    const size_t numFrames = sz/sizeof(Sample);

    for (unsigned i = 0; i < numFrames; i++) {
        if (buf[i] > max) {
            max = buf[i];
        }
    }

    unsigned numBars = (40*(max / 32600.0));

    char barsBuf[50];

    if (numBars >= 50) {
        numBars = 50 - 1;
    }

    memset(barsBuf, '\0', 50);
    memset(barsBuf, '|', numBars);

    // printf("%s\n", barsBuf);

    return AUDIO_CONTINUE;
}

static AudioCallbackResult producer(FrameBuffer buf, const size_t sz, void* callData) {
    for (unsigned i = 0; i < (sz/sizeof(Sample)); i++) {
        buf[i] = (Sample)(GeneratePinkNoise(&noise) * 32600.0 * 0.3);
    }

    nanosleep((const struct timespec[]){{0, 11610000 + (rand() % 100000)}}, NULL);
    return AUDIO_CONTINUE;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Please provide the audio device index as the first and only argument.\n");
        return 1;
    }

    int inpDevice = atoi(argv[1]);
    int outDevice = atoi(argv[2]);

    printf("Audio I/O will be bound to input device %d, output device %d.\n", inpDevice, outDevice);

    InitializePinkNoise(&noise, 12);

    Audio_init(inpDevice, outDevice);

    printf("Producing and consuming audio for 5 secs...\n");
    int res = Audio_start(producer, consumer, NULL);

    if (res != AUDIO_OK) {
        printf("Unable to start audio.\n");
        return 1;
    }

    printf("Audio started...\n");
    sleep(20);

    Audio_stop();

    printf("Done\n");

    Audio_teardown();
    return 0;
}
