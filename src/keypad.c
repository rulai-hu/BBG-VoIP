#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>

#include "../include/keypad.h"

#define DELAY_NS 100000000L // 100ms in 
#define BUFFER_SIZE 256
#define NUM_GPIO 12
#define DIAL_LENGTH 12
#define GPIO_BASE_DIR "/sys/class/gpio/"

static bool stop = false;
static bool printFlag = true;

static const long SECOND_NS = 1000000000L;

static const char *GPIO[] = {
		"71", "70", "75",
		"77", "76", "79",
		"78", "80", "8",
		"81", "9", "11"
};

static const char KEYS[] = {
		'3', '6', '9',
		'#', '2', '5',
		'8', '0', '1',
		'4', '7', '*'
};

void KEYPAD_init(void)
{
	FILE *file;
	char buffer[BUFFER_SIZE] = "";

	// Export GPIO pins
	for (int i = 0; i < NUM_GPIO; i++) 
	{
		file = fopen((GPIO_BASE_DIR "export"), "w");

		if (file == NULL) {
			perror("failed to open GPIO export file");
		}

		fprintf(file, "%s", GPIO[i]);
		fclose(file);
	}
	sleep(2);

	//nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);//sleep here ensures files get updated before access

	// Set GPIO directions to be input
	for(int i = 0; i < NUM_GPIO; ++i) 
	{
		strcpy(buffer, GPIO_BASE_DIR "gpio");
		strcat(buffer, GPIO[i]);
		strcat(buffer, "/direction");

		file = fopen(buffer, "w");
		if (file == NULL) {
			printf("Keypad: Failed to set GPIO pin %s.", GPIO[i]);
		}

		fprintf(file, "%s", "in");
		fclose(file);
	}

	// Ensure that files get updated before access
	//nanosleep((const struct timespec[]){{0, SECOND_NS/10L}}, NULL);//sleep here ensures files get updated before access
	sleep(1);
}


char * KEYPAD_getDial(void)
{
	FILE *file;
	char buffer[BUFFER_SIZE];
	char result[BUFFER_SIZE];
	char input[BUFFER_SIZE] = "";
	int iter = 0;

	printf("\nPlease enter the number you wish to dial(# to enter):\n\t");

	while(!stop)
	{
		// Sleep a 20th of a second
		nanosleep((const struct timespec[]){{0, 50000000L}}, NULL);
		for(int i = 0; i < NUM_GPIO; ++i) 
		{

			//read the value of the pins
			strcpy(buffer, GPIO_BASE_DIR "gpio");
			strcat(buffer, GPIO[i]);
			strcat(buffer, "/value");

			file = fopen(buffer, "r");
			if (file == NULL) {
				printf("failed read %s", GPIO[i]);
			}

			fgets(result, BUFFER_SIZE, file);
			fclose(file);

			printFlag = true;
			while(result[0] == '1')
			{
				file = fopen(buffer, "r");
				if (file == NULL) 
				{
					printf("failed read %s", GPIO[i]);
				}
				fgets(result, BUFFER_SIZE, file);
				fclose(file);

				if(i == 3)
				{
					stop = true;
					printf("\n");
					break;
				}

				if(printFlag)
				{
					// input = keys

					sprintf(input,"%s%c", input, KEYS[i]);
					printf("%c", KEYS[i]);
					fflush(stdout);
					//printf("%s", input);
					printFlag = false;
					iter++;
					if(iter == DIAL_LENGTH)
					{
						stop = true;
						printf("\n");
						break;
					}
				}
			}
		}
	}
	//printf("sending input = %s\n", input);
	
	char * ret_string;
    ret_string = malloc(sizeof(char) * DIAL_LENGTH);
	
	if(iter == 0)
	{
		sprintf(ret_string, "-1");
	}

	sprintf(ret_string, "%s", input);
	//printf("sending input = %s\n", ret_string);

	return(ret_string);
}