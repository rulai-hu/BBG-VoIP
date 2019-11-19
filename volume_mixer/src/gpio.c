// This C source file provides utility functions to access and modify the
// Beaglebone GPIO pins.

#include <stdio.h>
#include <stdlib.h>

#include "../include/file.h"

#include "../include/gpio.h"

const unsigned int MAX_STR_LEN = 1024;

// ENUMS

typedef enum {
  off = 1,
  on = 0
} joystickStatus;

// Access functions

void Gpio_getString(
    const unsigned int pin,
    char* buffer,
    const unsigned int max_length) {

  // Format pin into string filename
  char filename[MAX_STR_LEN];
  sprintf(filename, "/sys/class/gpio/gpio%d/value", pin);

  readFromFile(filename, buffer, max_length);
}

int Gpio_getInt(const unsigned int pin) {
  char buffer[MAX_STR_LEN];
  Gpio_getString(pin, buffer, MAX_STR_LEN);
  return atoi(buffer);
}

// Modification functions

void Gpio_enablePin(const unsigned int pin) {
  writeIntToFile("/sys/class/gpio/export", pin);
}

void Gpio_enablePinOutput(const unsigned int pin) {
  Gpio_enablePin(pin);

  char filename[1024];
  sprintf(filename, "/sys/class/gpio/gpio%d/direction", pin);
  writeStringToFile(filename, "out");
}

void Gpio_disablePin(const unsigned int pin) {
  writeIntToFile("/sys/class/gpio/unexport", pin);
}

void Gpio_setValue(const unsigned int pin, const int value) {
  // Format pin into string filename
  char filename[1024];
  sprintf(filename, "/sys/class/gpio/gpio%d/value", pin);

  writeIntToFile(filename, value);
}
