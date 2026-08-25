#ifndef ENKODER_H
#define ENKODER_H

#include "pico/stdlib.h"

extern volatile int counter;
extern volatile bool button_pressed;
extern volatile bool last_state_A;

void encoder_init();

void encoder_isr(uint gpio, uint32_t events);

#endif
