#include <sys/socket.h>
#include <stdio.h>

#include "include/connection.h"
#include "include/audio.h"
#include "include/trek.h"

#define NUM_FRAMES 95616
#define NUM_BUFFERS ((unsigned int)NUM_FRAMES / FRAMES_PER_BUFFER)
#define FRAME_PLAYBACK_TIME_NS ((long) (FRAME_PLAYBACK_TIME * 1e9))

static void playWAVFile(FrameBuffer);
static Sample framebuffer[FRAMES_PER_BUFFER];
static FILE* wav;

void Trek_play(Connection* conn) {
    wav = fopen("initSig.wav", "r");


    if (wav == NULL) {
        perror("Trek");
        exit(1);
    }

    fseek(wav, 44, SEEK_SET);
    // Audio_playBuffers(playWAVFile, NUM_BUFFERS);

    for (int j = 0; j < NUM_BUFFERS; j++) {
        fread(framebuffer, sizeof(Sample), FRAMES_PER_BUFFER, wav);
        send(conn->socket, framebuffer, FRAMEBUFFER_SIZE, 0);

        // nanosleep((const struct timespec[]){{0, FRAME_PLAYBACK_TIME_NS}}, NULL);
    }

    fclose(wav);
}

void playWAVFile(FrameBuffer buf) {
    fread(buf, sizeof(Sample), FRAMES_PER_BUFFER, wav);
}
