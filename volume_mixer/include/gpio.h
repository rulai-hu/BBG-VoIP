#ifndef GPIO
#define GPIO

void Gpio_getString(
    const unsigned int pin,
    char* buffer,
    const unsigned int max_length);

int Gpio_getInt(const unsigned int pin);

void Gpio_enablePin(const unsigned int pin);

void Gpio_enablePinOutput(const unsigned int pin);

void Gpio_disablePin(const unsigned int pin);

void Gpio_setValue(const unsigned int pin, const int value);

#endif
