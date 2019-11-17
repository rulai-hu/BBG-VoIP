void volume_init();

void volume_cleanup();

void volume_thread_runner();

int get_new_volume();

int readVoltageRawFromChannel(unsigned int channel);

void set_volume(long volume);
long reading_to_volume(int reading);