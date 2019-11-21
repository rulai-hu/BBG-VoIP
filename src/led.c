#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../include/led.h"

// GPIO index and path of each LED
#define RED 0
#define BLUE 1
static const char *GPIO[] = {"46", "26"};
#define GPIO_DIR "/sys/class/gpio/"

#define NUM_LIGHTS 2
#define BUFFER_SIZE 256

#define BLINK_DURATION 100000000

pthread_t blink_thread;
_Bool blink_thread_running = false;

static FILE* openFile(const char* path, const char* symbol);
static void delay(const unsigned int nanosecs);

void LED_init(void)
{
	FILE *file;
	char buffer[BUFFER_SIZE] = "";

	// Eport GPIO pins
	for (int i = 0; i < NUM_LIGHTS; i++) {
		file = fopen((GPIO_DIR "export"), "w");

		if (file == NULL) {
			perror("LED: Failed to open GPIO export file");
		}

		fprintf(file, "%s", GPIO[i]);
		fclose(file);
	}

	sleep(1);
	//Set directions as in
	for(int i = 0; i < NUM_LIGHTS; ++i) {
		strcpy(buffer, GPIO_DIR "gpio");
		strcat(buffer, GPIO[i]);
		strcat(buffer, "/direction");

		file = fopen(buffer, "w");

		if (file == NULL) {
			printf("LED: Failed to set GPIO pin %s", GPIO[i]);
		}

		fprintf(file, "%s", "out");
		fclose(file);
	}
	nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
}


static void LED_on(int colourIdx)
{
	//printf("%s\n", GPIO[RED]);
	int charWritten = 99;
	char buffer[BUFFER_SIZE] = "";
	strcpy(buffer, GPIO_DIR "gpio");
	strcat(buffer, GPIO[colourIdx]);
	strcat(buffer, "/value");

	FILE* LEDPointer = openFile(buffer, "w");
	charWritten = fprintf(LEDPointer, "1");
	if (charWritten <= 0)
    {
		printf("LED: ERROR WRITING DATA.");
		exit(1);
	}

	fclose(LEDPointer);
}

static void LED_off(int colourIdx)
{
	int charWritten = 99;
	char buffer[BUFFER_SIZE] = "";
	strcpy(buffer, GPIO_DIR "gpio");
	strcat(buffer, GPIO[colourIdx]);
	strcat(buffer, "/value");

	FILE* LEDPointer = openFile(buffer, "w");
	charWritten = fprintf(LEDPointer, "0");
	if (charWritten <= 0)
    {
		printf("LED: ERROR WRITING DATA.");
		exit(1);
	}

	fclose(LEDPointer);
}

void LED_red_on(void)
{
	LED_on(RED);
}
void LED_red_off(void)
{
	LED_off(RED);
}

void LED_blu_on(void)
{
	LED_on(BLUE);
}
void LED_blu_off(void)
{
	LED_off(BLUE);
}

static FILE* openFile(const char* path, const char* symbol) {
	FILE* filePointer  = fopen(path, symbol);

	if (filePointer == NULL)
	{
		printf("ERROR OPENING %s.", path);
		exit(1);
	}
	return filePointer;
}

static void delay(const unsigned int nanosecs) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = nanosecs;
  nanosleep(&t, NULL);
}

static void *led_red_blink_loop() {
	while (1) {
		LED_red_on();
		delay(BLINK_DURATION);
		LED_red_off();
		delay(BLINK_DURATION);
	}
	return NULL;
}

void LED_red_blink_start(void) {
	if (blink_thread_running) {
		printf("LED: The blink thread already running.");
		return;
	}
	blink_thread_running = true;

	pthread_attr_t attrs;
	pthread_attr_init(&attrs);
	pthread_create(&blink_thread, &attrs, led_red_blink_loop, NULL);
	pthread_attr_destroy(&attrs);
}


void LED_red_blink_stop(void) {
	if (!blink_thread_running) {
		printf("LED: Cannot stop the blink thread because it is not running.");
		return;
	}
	blink_thread_running = false;

	pthread_cancel(blink_thread);
	pthread_join(blink_thread, NULL);
}