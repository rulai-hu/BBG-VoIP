#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define DELAY_NS 100000000L // 100ms in 
#define BUFFER_SIZE 256
#define NUM_GPIO 12
#define GPIO_BASE_DIR "/sys/class/gpio/"

static const char *GPIO[] = {"71", "70", "75", "77",  "76", "79", "78", "80", "8", "81", "9", "11"};
static const char KEYS[] = { '3', '6', '9', '#', '2', '5', '8', '0', '1', '4', '7', '*'};

short getDial(void)
{
	
}