//testing file for keyboard input
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../include/keypad.h"

int main() 
{
	KEYPAD_init();
	short num = KEYPAD_getDial();

	printf("\nNumber entered: %d\n", num);
	sleep(1);
	return 0;
}
