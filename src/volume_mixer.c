#include "../include/volume_mixer.h"
#include "../include/file.h"

#include <alsa/asoundlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <math.h>

static const char *VOLTAGE_FILE = "/sys/bus/iio/devices/iio:device0/in_voltage%d_raw";
static const int BUFFER_MAX_LEN = 1024;

int currentVolume;
// Volume threading
void* volumeThread();
static _Bool stopping = false;
static pthread_t volumeThreadID;

void volume_init()
{
    pthread_create(&volumeThreadID, NULL, volumeThread, NULL);
}

void volume_cleanup()
{
    // Stop the Volume Mixer thread
	stopping = true;
	pthread_join(volumeThreadID, NULL);

}

void* volumeThread()
{
	while (!stopping) {
        int reading = readVoltageRawFromChannel(0);
        int volume = reading_to_volume(reading);
        set_volume(volume);
	}

	return NULL;
}

int reading_to_volume(int reading)
{
    double volume_precise = (reading * 100.00) / 4095;
    int volume_rounded = round(volume_precise);
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

void set_volume(int volume)
{

	printf("Volume: %d\n", volume);

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

    snd_mixer_close(handle);
}

void main()
{
    printf("Starting mixer\n");
    volume_init();

    while (true) {
        ;
    }
}