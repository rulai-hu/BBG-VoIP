//testing file for LED capabilities
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../include/led.h"

int main()
{
	LED_init();
	sleep(3);
	LED_red_on();
	sleep(3);
	LED_red_off();
	sleep(3);
	LED_blu_on();
	sleep(3);
	LED_blu_off();
	sleep(3);
}
