#include "../include/volume_mixer.h"
#include "../include/file.h"
#include "../include/i2c.h"

#include <alsa/asoundlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <math.h>
#include <signal.h>
#include <alloca.h>

static const char *VOLTAGE_FILE = "/sys/bus/iio/devices/iio:device0/in_voltage%d_raw";
static const int BUFFER_MAX_LEN = 1024;

bool volatile keepRunning = true;
long currentVolume = 0;

static _Bool stoppingVolume = false;
static pthread_t volumeThreadID;
static _Bool stoppingI2C = false;
static pthread_t i2cThreadID;

int main()
{
    printf("Starting volume control.\n");
    volume_init();
    signal(SIGINT, boolHandler);

    while (keepRunning) ;
    printf("Stopping volume control.\n");
    volume_cleanup();
    return 0;
}

void boolHandler()
{
    keepRunning = false;
}

void volume_init()
{
    initializeI2cBus();
    pthread_create(&volumeThreadID, NULL, volumeThread, NULL);
    pthread_create(&i2cThreadID, NULL, i2cThread, NULL);
}

void volume_cleanup()
{
    // Stop the Volume Mixer thread
	stoppingVolume = true;
	pthread_join(volumeThreadID, NULL);

    stoppingI2C = true;
	pthread_join(i2cThreadID, NULL);

    disableDigits();
    closeI2cBus();
}

void* volumeThread()
{
	while (!stoppingVolume) {
        int reading = readVoltageRawFromChannel(0);
        long volume = reading_to_volume(reading);
        set_volume(volume);
	}

	return NULL;
}

void* i2cThread()
{
	while (!stoppingI2C) {
        writeTwoDigitsToI2cRegister(currentVolume, 7000000);
	}

	return NULL;
}

long reading_to_volume(int reading)
{
    long volume_precise = (reading * 100.00) / 4095;
    long volume_rounded = round(volume_precise);
    return volume_rounded;
}

int readVoltageRawFromChannel(unsigned int channel) {
    // Create name of file to read from
    char filename[BUFFER_MAX_LEN];
    sprintf(filename, VOLTAGE_FILE, channel);

    // Read value from file
    char read_voltage_raw[BUFFER_MAX_LEN];
    readFromFile(filename, read_voltage_raw, BUFFER_MAX_LEN);

    return atoi(read_voltage_raw);
}


void set_volume(long volume)
{

	if (volume != currentVolume) {
        
        printf("Changing volume to : %ld\n", volume);

        long min, max;
        snd_mixer_t *handle;
        snd_mixer_selem_id_t *sid;
        const char *card = "default";
        const char *selem_name = "PCM";

        snd_mixer_open(&handle, 0);
        snd_mixer_attach(handle, card);
        snd_mixer_selem_register(handle, NULL, NULL);
        snd_mixer_load(handle);

        snd_mixer_selem_id_alloca(&sid);
        snd_mixer_selem_id_set_index(sid, 0);
        snd_mixer_selem_id_set_name(sid, selem_name);
        snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

        snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
        snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);
        currentVolume = volume;

        snd_mixer_close(handle);
    }
}


