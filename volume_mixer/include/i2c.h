// This C module provides utility functions to access and control the
// I2C (Inter-Integrated-Circuit) on the BeagleBone.
//
// Before using these functions, the following commands must be executed on
// the BeagleBone:
//   config-pin P9_17 i2c
//   config-pin P9_18 i2c


#ifndef I2C_H
#define I2C_H

typedef enum {
  REGISTER_LOWER,
  REGISTER_UPPER,
  REGISTER_OUTPUT_1,
  REGISTER_OUTPUT_2
} Register;

void initializeI2cBus();

void disableDigits();

// Will not explicitly disable the other digit.
void enableLeftDigit();

// Will not explicitly disable the other digit.
void enableRightDigit();

void writeDigitToI2cRegister(unsigned int digit);

// Must be looped to continually show the value.
// Only takes the last two digits of the number.
// Does not show no leading 0s.
void writeTwoDigitsToI2cRegister(unsigned int num, unsigned int nanosecsDelay);

// Takes hexadecimal values which represent half of the
// segments from the display.
void writeValueToI2cRegister(Register r, unsigned char value);

void closeI2cBus();

#endif
