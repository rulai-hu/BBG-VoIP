// This C module provides functions to turn red and blue LEDs on and off.

#ifndef LED_H
#define LED_H

// This function sets the GPIO outputs for the LEDs.
void LED_init(void);

// This function turns on the red LED until it is manually turned off.
// Must have called LED_init() beforehand.
void LED_red_on(void);

// This function turns off the red LED until it is manually turned on.
// Must have called LED_init() beforehand.
void LED_red_off(void);

// This function turns on the blue LED until it is manually turned off.
// Must have called LED_init() beforehand.
void LED_blu_on(void);

// This function turns off the blue LED until it is manually turned on.
// Must have called LED_init() beforehand.
void LED_blu_off(void);

#endif
