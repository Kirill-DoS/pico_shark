#include "include/joystick.h"

void joystick_init(uint pin){
    adc_init();
    adc_gpio_init(pin);
}

void read_joystick(pin){
    adc_select_input(0);
    uint16_t x_val = adc_read();
}
