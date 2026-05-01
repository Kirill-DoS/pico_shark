// enkoder.cpp
#include "include/enkoder.h"
#include "hardware/gpio.h"

volatile int counter = 0;
bool last_state_A = false;

void encoder_init(){
    gpio_init(DT);
    gpio_init(CLK);
    gpio_init(SW);

    gpio_set_dir(DT, GPIO_IN);
    gpio_set_dir(CLK, GPIO_IN);
    gpio_set_dir(SW, GPIO_IN);

    gpio_pull_up(DT);
    gpio_pull_up(CLK);
    gpio_pull_up(SW);

    last_state_A = gpio_get(DT);
}

void encoder_isr(uint gpio, uint32_t events){
    bool current_state_A = gpio_get(DT);

    if(current_state_A != last_state_A){
        bool state_B = gpio_get(CLK);

        if(state_B != current_state_A){
            counter++;
        }else{
            counter--;
        }
    }

    last_state_A = current_state_A;
}
