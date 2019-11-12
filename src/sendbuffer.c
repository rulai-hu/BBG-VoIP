#include <stdlib.h>

#include "include/sendbuffer.h"
#include "include/ringbuffer.h"

static RingBuffer* buffer;

void SendBuffer_init() {
    buffer = malloc(sizeof(RingBuffer));
    RingBuffer_init(buffer, SENDBUFFER_MAX_NUM_FRAMES, sizeof(Sample));
}

void SendBuffer_destroy() {
    free(buffer);
}

int SendBuffer_write(Sample* input, int size) {

    return 0;
}

int SendBuffer_writeConstant(Sample val, int size) {
    return 0;
}
