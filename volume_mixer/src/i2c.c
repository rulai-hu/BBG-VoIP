// This C source file provides utility functions to access and control the
// I2C (Inter-Integrated-Circuit) on the BeagleBone.

#include <fcntl.h>  // For O_RDWR
#include <linux/i2c.h>
#include <linux/i2c-dev.h>  // For I2C_SLAVE
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "../include/gpio.h"

#include "../include/i2c.h"

typedef enum {
  BUS0,
  BUS1,
  BUS2
} I2cBus;

static const int BUFFER_LEN = 1024;

static const char PORT_VAL_OUTPUT = 0x00;

static const int PIN_LEFT = 61;
static const int PIN_RIGHT = 44;

static const char I2C_DEVICE_ADDRESS = 0x20;

// Stores all opened file descriptors.
static int BusFileDescriptors[3] = {0, 0, 0};

static void delay(const unsigned int nanosecs) {
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = nanosecs;
  nanosleep(&t, NULL);
}

static void getFileOfBus(I2cBus b, char *filename) {
  sprintf(filename, "/dev/i2c-%d", b);
}

static unsigned char toRegisterAddress(Register r) {
  if (r == REGISTER_LOWER) {
    return 0x14;
  }
  else if (r == REGISTER_UPPER) {
    return 0x15;
  }
  else if (r == REGISTER_OUTPUT_1) {
    return 0x00;
  }
  else if (r == REGISTER_OUTPUT_2) {
    return 0x01;
  }
  return 0;
}

void initializeI2cBus() {
  // Turn on I2C configurations for the pins
  // system("config-pin P9_17 i2c");
  // system("config-pin P9_18 i2c");

  // Export, set to output, and turn on
  Gpio_enablePinOutput(PIN_LEFT);
  Gpio_enablePinOutput(PIN_RIGHT);
  Gpio_setValue(PIN_LEFT, 1);
  Gpio_setValue(PIN_RIGHT, 1);

  // Initialize bus
  char filename[BUFFER_LEN];
  getFileOfBus(BUS1, filename);

  int fileDescriptor = open(filename, O_RDWR);
  int result = ioctl(fileDescriptor, I2C_SLAVE, I2C_DEVICE_ADDRESS);

  if (result < 0) {
    perror("Unable to set I2C device to slave address.");
    exit(1);
  }

  // Set to array value in static variables
  BusFileDescriptors[BUS1] = fileDescriptor;

  // Enable I2C outputs
  writeValueToI2cRegister(REGISTER_OUTPUT_1, PORT_VAL_OUTPUT);
  writeValueToI2cRegister(REGISTER_OUTPUT_2, PORT_VAL_OUTPUT);
}

void disableDigits() {
  Gpio_setValue(PIN_LEFT, 0);
  Gpio_setValue(PIN_RIGHT, 0);
}

void enableLeftDigit() {
  // Enable pins as GPIO, set as output, and enable value
  Gpio_enablePinOutput(PIN_LEFT);
  Gpio_setValue(PIN_LEFT, 1);
}

void enableRightDigit() {
  // Enable pins as GPIO, set as output, and enable value
  Gpio_enablePinOutput(PIN_RIGHT);
  Gpio_setValue(PIN_RIGHT, 1);
}

void writeDigitToI2cRegister(unsigned int digit) {
  // Take only rightmost digit
  if (digit > 9) {
    digit %= 10;
  }

  unsigned char upper = 0;
  unsigned char lower = 0;

  // Set to values which display a digit
  switch (digit) {
    case 0:
      upper = 0x86;
      lower = 0xA1;
      break;
    case 1:
      upper = 0x02;
      lower = 0x80;
      break;
    case 2:
      upper = 0x0E;
      lower = 0x31;
      break;
    case 3:
      upper = 0x0E;
      lower = 0xB0;
      break;
    case 4:
      upper = 0x8A;
      lower = 0x90;
      break;
    case 5:
      upper = 0x8C;
      lower = 0xB0;
      break;
    case 6:
      upper = 0x8C;
      lower = 0xB1;
      break;
    case 7:
      upper = 0x06;
      lower = 0x80;
      break;
    case 8:
      upper = 0x8E;
      lower = 0xB1;
      break;
    case 9:
    default:
      upper = 0x8E;
      lower = 0x90;
  }

  writeValueToI2cRegister(REGISTER_UPPER, upper);
  writeValueToI2cRegister(REGISTER_LOWER, lower);
}

void writeTwoDigitsToI2cRegister(unsigned int num, unsigned int nanosecsDelay) {
  // Cap at 99
  num = (num > 99) ? 99 : num;
  unsigned int digit1 = num/10;
  unsigned int digit2 = num % 10;

  disableDigits();
  enableLeftDigit();
  writeDigitToI2cRegister(digit1);
  delay(nanosecsDelay);

  disableDigits();
  enableRightDigit();
  writeDigitToI2cRegister(digit2);
  delay(nanosecsDelay);
}

void writeValueToI2cRegister(Register r, unsigned char value) {  
  int registerBufferLen = 2;
  unsigned char buffer[2] = {toRegisterAddress(r), value};

  int result = write(BusFileDescriptors[BUS1], buffer, registerBufferLen);
  if (result != registerBufferLen) {
    char msg[BUFFER_LEN];
    sprintf(msg, "Unable to write to I2C bus %d, register %u.", BUS1, r);
    perror(msg);
    exit(1);
  }
}

void closeI2cBus() {
  close(BusFileDescriptors[BUS1]);
}