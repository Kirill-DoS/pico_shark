// enkoder.cpp
#include "include/enkoder.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdio.h>

volatile int counter = 0;
volatile bool button_pressed = false; // Флаг фиксации нажатия кнопки
bool last_state_A = false;

// Объявляем ISR заранее
void encoder_isr(uint gpio, uint32_t events);

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

    // Включаем прерывания на вращение (DT)
    gpio_set_irq_enabled_with_callback(DT, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &encoder_isr);
    // Добавляем пин кнопки (SW) в ту же самую систему прерываний процессора!
    gpio_set_irq_enabled(SW, GPIO_IRQ_EDGE_FALL, true);
}

void encoder_isr(uint gpio, uint32_t events){
    uint32_t current_time = time_us_32();
    static uint32_t last_interrupt_time = 0;

    // Защита от дребезга контактов (и для кручения, и для кнопки)
    if (current_time - last_interrupt_time < 20000) { // 20 мс дебаунс
        return;
    }
    last_interrupt_time = current_time;

    // Если прерывание пришло от кнопки (SW)
    if (gpio == SW) {
        button_pressed = true; // Фиксируем аппаратное нажатие!
        printf("Hardware SW ISR Triggered!\n");
        return;
    }

    // Логика обработки вращения (DT)
    bool current_state_A = gpio_get(DT);
    if(current_state_A != last_state_A){
        bool state_B = gpio_get(CLK);
        if(state_B != current_state_A){
            counter++;
        } else {
            counter--;
        }
        last_state_A = current_state_A;
    }
}
