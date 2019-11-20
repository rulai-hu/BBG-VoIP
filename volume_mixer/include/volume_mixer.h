#ifndef VOLUME_MIXER_H
#define VOLUME_MIXER_H

// Thread functions for volume and i2c
void* volumeThread();
void* i2cThread();

// SIGINT listener
void boolHandler();

// Init volume control
void volume_init();

// Cleanup volume control
void volume_cleanup();

// Set ALSA volume
void set_volume(long volume);

// Read potentiometer value
int readVoltageRawFromChannel(unsigned int channel);

// Convert potentiometer reading value to volume value
long reading_to_volume(int reading);

#endif