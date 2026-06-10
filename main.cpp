#include "setup.h"

#include <cstddef>
#include <cstdint>
#include <stdio.h>


int main() {
    stdio_init_all();
    sleep_ms(2000); // Даем время на инициализацию USB

    setup_all();

    while (true) {
        lv_tick_inc(5);
        lv_timer_handler();
        sleep_ms(5);
    }
}
