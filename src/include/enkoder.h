#ifndef ENKODER_H
#define ENKODER_H

#include "pico/stdlib.h"

#define SW  17
#define DT 16
#define CLK 15

extern volatile int counter;
extern volatile bool button_pressed;
extern bool last_state_A;

void encoder_init();

void encoder_isr(uint gpio, uint32_t events);

#endif
