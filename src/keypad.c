#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define DELAY_NS 100000000L // 100ms in 
#define BUFFER_SIZE 256
#define NUM_GPIO 12
#define GPIO_BASE_DIR "/sys/class/gpio/"

static bool stop = false;
static bool printFlag = true;

static const char *GPIO[] = {"71", "70", "75", "77",  "76", "79", "78", "80", "8", "81", "9", "11"};
static const char KEYS[] = {'3', '6', '9', '#', '2', '5', '8', '0', '1', '4', '7', '*'};

void KEYPAD_init(void)
{
	FILE *file;
	char buffer[BUFFER_SIZE] = "";

	//export GPIO pins
	for (int i = 0; i < NUM_GPIO; i++) {
		file = fopen((GPIO_BASE_DIR "export"), "w");

		if (file == NULL) {
			perror("failed to open GPIO export file");
		}

		fprintf(file, "%s", GPIO[i]);
		fclose(file);
	}

	sleep(1);//sleep here ensures files get updated before access
	//Set directions as in
	for(int i = 0; i < NUM_GPIO; ++i) {
		strcpy(buffer, GPIO_BASE_DIR "gpio");
		strcat(buffer, GPIO[i]);
		strcat(buffer, "/direction");

		file = fopen(buffer, "w");

		if (file == NULL) {
			printf("failed to set GPIO pin %s", GPIO[i]);
		}

		fprintf(file, "%s", "in");
		fclose(file);
	}
	sleep(1);//sleep here ensures files get updated before access
}


short KEYPAD_getDial(void)
{
	FILE *file;
	char buffer[BUFFER_SIZE];
	char result[BUFFER_SIZE];
	char input[BUFFER_SIZE] = "";
	//input = "";
	int iter = 0;

	printf("\nPlease enter the number you wish to dial(# to enter):\n\t");

	while(!stop)
	{
		//sleep a 20th of a second
		nanosleep((const struct timespec[]){{0, 100000000L}}, NULL);
		for(int i = 0; i < NUM_GPIO -1; ++i) 
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
					if(iter == 12)
					{
						stop = true;
						printf("\n");
						break;
					}
				}
			}
		}
	}
	printf("sending input = %s", input);
	if(iter == 0)
	{
		return -1;
	} else {
		return atoi(input);
	}
}