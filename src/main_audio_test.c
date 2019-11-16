#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "include/audio.h"
#include "include/pink.h"

static PinkNoise noise;

static AudioCallbackResult consumer(FrameBuffer buf, const size_t sz) {
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

    printf("%s\n", barsBuf);

    return AUDIO_CONTINUE_RECORDING;
}

static AudioCallbackResult producer(FrameBuffer buf, const size_t sz) {
    for (unsigned i = 0; i < (sz/sizeof(Sample)); i++) {
        buf[i] = (Sample)(GeneratePinkNoise(&noise) * 32600.0 * 0.3);
    }

    nanosleep((const struct timespec[]){{0, 11610000 + (rand() % 100000)}}, NULL);
    return AUDIO_CONTINUE_PLAYBACK;
}

int main(void) {
    InitializePinkNoise(&noise, 12);

    Audio_init();

    printf("Producing and consuming audio for 5 secs...\n");
    int x = Audio_start(producer, consumer);

    sleep(5);

    Audio_stop();

    printf("Done\n");

    Audio_teardown();
    return 0;
}
