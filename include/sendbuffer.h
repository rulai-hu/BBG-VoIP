#ifndef SENDBUFFER_H
#define SENDBUFFER_H

// can buffer up to 1 sec of input
#define SENDBUFFER_MAX_NUM_FRAMES 44100

typedef short Sample;

// #include "include/portaudio.h"
void SendBuffer_init(void);
void destroy(void);
int SendBuffer_write(Sample*, int);
int SendBuffer_writeConstant(Sample, int);

#endif
