#ifndef ENKODER_H
#define ENKODER_H

#include "pico/stdlib.h"

#define SW  9
#define DT 12
#define CLK 13

extern volatile int counter;
extern bool last_state_A;

void encoder_init();

void encoder_isr(uint gpio, uint32_t events);

#endif
