// This C module manages the input from a 12-digit keypad.
//
// Keypad information:
//   SKU: 12KEY
//   https://www.rpelectronics.com/12key-12-key-keypad-common-ground.html

#ifndef KEY_HEADER
#define KEY_HEADER

// This function initializes the GPIO pins for the keypad.
// See the schematic in the project directory for instructions on wiring.
void KEYPAD_init(void);

// This function retrieves the key which is currently pressed down.
//
// The user must free the allocated memory used to return the char.
//
// Only guaranteed to return correct input when only 1 key is pressed.
// Results may vary if multiple keys are pressed simultaneously.
const char * KEYPAD_getDial(void);

#endif