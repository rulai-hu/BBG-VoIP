#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H

typedef short Sample;

// #include "include/portaudio.h"
int AudioBuffer_write(Sample*, int);
int AudioBuffer_writeConstant(Sample, int);

#endif
