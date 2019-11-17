#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define YELLOW = 0
#define RED = 1
#define BLUE = 2
#define GPIO_DIR = "/sys/class/gpio"
#define NUM_LIGHTS = 3
#define BUFFER_SIZE = 256

static const char *GPIO[] = {"26", "46", "47"};

void LED_init()
{
	FILE *file;
	char buffer[BUFFER_SIZE] = "";

	//export GPIO pins
	for (int i = 0; i < NUM_LIGHTS; i++) {
		file = fopen((GPIO_DIR "export"), "w");

		if (file == NULL) {
			perror("failed to open GPIO export file");
		}

		fprintf(file, "%s", GPIO[i]);
		fclose(file);
	}

	nanosleep((const struct timespec[]){{0, 300000000L}}, NULL);//sleep here ensures files get updated before access
	//Set directions as in
	for(int i = 0; i < LIGHTS; ++i) {
		strcpy(buffer, GPIO_DIR "gpio");
		strcat(buffer, GPIO[i]);
		strcat(buffer, "/direction");

		file = fopen(buffer, "w");

		if (file == NULL) {
			printf("failed to set GPIO pin %s", GPIO[i]);
		}

		fprintf(file, "%s", "out");
		fclose(file);
	}
	nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
}

static void LED_flash(int gpio)
{
	return;
}

static void LED_on(int gpio)
{
	return;
}

static void LED_off(int gpio)
{

}
/*
void LED_red_flash(void);
void LED_red_on(void);
void LED_red_off(void);

void LED_blu_flash(void);
void LED_blu_on(void);
void LED_blu_off(void);

void LED_yel_flash(void);
void LED_yel_on(void);
void LED_yel_off(void);
*/