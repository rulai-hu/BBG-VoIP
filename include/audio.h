// This C module file provides functions for continuous audio creation/playback.

#ifndef AUDIO_H
#define AUDIO_H

#include <stdlib.h>

#define Sample short
typedef Sample* FrameBuffer;

#define SILENCE             0
#define FRAMES_PER_BUFFER   512
#define SAMPLE_RATE         44100
#define FRAME_PLAYBACK_TIME ((double) FRAMES_PER_BUFFER / SAMPLE_RATE)
#define FRAMEBUFFER_SIZE    FRAMES_PER_BUFFER * sizeof(Sample) // valid for ONE channel

// 1 sec playback time. Multiply by a constant n to get n secs.
#define MIN_PLAYBACK_QUEUE_LENGTH ((unsigned) 20)

// Enum types for actions from the function callbacks
typedef enum {
    AUDIO_CONTINUE = 0,
    AUDIO_STOP
} AudioCallbackResult;

// Function type which produces framebuffers for playback.
typedef AudioCallbackResult (*AudioProducer)(FrameBuffer, const size_t, void*);

// Function type which consumes framebuffers produced by the audio device.
typedef AudioCallbackResult (*AudioConsumer)(FrameBuffer, const size_t, void*);

typedef void (*AudioGenerator)(FrameBuffer);

// Enum types for actions from the functions
typedef enum {
    AUDIO_OK = 0,
    AUDIO_NOT_INITIALIZED,
    AUDIO_OPEN_STREAM_FAIL,
    AUDIO_START_STREAM_FAIL,
    AUDIO_CLOSE_STREAM_FAIL,
    AUDIO_ALREADY_STARTED,
    AUDIO_ALREADY_STOPPED
} AudioResult;

// This function initializes the devices and structures required for audio.
// Must be called before any of the functions below.
void Audio_init(int, int);

// This function creates new threads which begin playing and recording audio.
//
// If init() has not been called, AUDIO_NOT_INITIALIZED will be returned.
// If audio has already been started previously,
//   AUDIO_ALREADY_STARTED will be returned.
AudioResult Audio_start(AudioProducer, AudioConsumer, void*);

// This function stops the threads which are playing and recording audio.
//
// If init() has not been called, AUDIO_NOT_INITIALIZED will be returned.
// If audio has not been started or has been stopped previously,
//   AUDIO_ALREADY_STOPPED will be returned.
AudioResult Audio_stop(void);

// This function clears the audio buffers and frees allocated resources.
void Audio_teardown(void);

void Audio_playBuffers(AudioGenerator, int);

#endif
