#ifndef AUDIO_H
#define AUDIO_H

#include <stdlib.h>

#define Sample short
typedef Sample* FrameBuffer;

typedef void (*AudioProducer)(FrameBuffer, const size_t);
typedef void (*AudioConsumer)(FrameBuffer, const size_t);

typedef enum {
    AUDIO_OK = 0, AUDIO_NOT_INITIALIZED, AUDIO_OPEN_STREAM_FAIL, AUDIO_START_STREAM_FAIL, AUDIO_CLOSE_STREAM_FAIL
} AudioResult;

void Audio_init(void);
void Audio_teardown(void);
AudioResult Audio_start(AudioProducer, AudioConsumer);
AudioResult Audio_stop(void);

#endif
