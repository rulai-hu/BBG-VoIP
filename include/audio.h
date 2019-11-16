#ifndef AUDIO_H
#define AUDIO_H

#include <stdlib.h>

#define Sample short
typedef Sample* FrameBuffer;

typedef enum {
    AUDIO_CONTINUE_RECORDING = 0, AUDIO_CONTINUE_PLAYBACK, AUDIO_STOP_RECORDING, AUDIO_STOP_PLAYBACK
} AudioCallbackResult;

// Produces framebuffers for playback
typedef AudioCallbackResult (*AudioProducer)(FrameBuffer, const size_t);
// Consumes framebuffers produced by the audio device
typedef AudioCallbackResult (*AudioConsumer)(FrameBuffer, const size_t);

typedef enum {
    AUDIO_OK = 0, AUDIO_NOT_INITIALIZED, AUDIO_OPEN_STREAM_FAIL, AUDIO_START_STREAM_FAIL, AUDIO_CLOSE_STREAM_FAIL
} AudioResult;

void Audio_init(void);
void Audio_teardown(void);
AudioResult Audio_start(AudioProducer, AudioConsumer);
AudioResult Audio_stop(void);

#endif
