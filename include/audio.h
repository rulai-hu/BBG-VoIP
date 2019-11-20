// This C module file provides functions for continuous audio creation/playback.

#ifndef AUDIO_H
#define AUDIO_H

#include <stdlib.h>

#define Sample short
typedef Sample* FrameBuffer;

// Enum types for actions from the function callbacks
typedef enum {
    AUDIO_CONTINUE_RECORDING = 0,
    AUDIO_CONTINUE_PLAYBACK,
    AUDIO_STOP_RECORDING,
    AUDIO_STOP_PLAYBACK
} AudioCallbackResult;

// Function type which produces framebuffers for playback.
typedef AudioCallbackResult (*AudioProducer)(FrameBuffer, const size_t, void*);

// Function type which consumes framebuffers produced by the audio device.
typedef AudioCallbackResult (*AudioConsumer)(FrameBuffer, const size_t, void*);

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
void Audio_init(int);

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

#endif
