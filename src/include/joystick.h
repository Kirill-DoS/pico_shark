#ifndef JOYSTICK_H
#define JOYSTICK_H

#include "pico/stdlib.h"
#include "hardware/adc.h"

// init
void joystick_init(uint pin);

// read value
void read_joystick(uint pin);
#endif
