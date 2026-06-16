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

        if (wifi_scan_finished) {
            wifi_scan_finished = false; // Сброс флага
            is_scanning_now = false;    // Сброс блокировки кнопки
            lv_roller_set_options(ui_NetListRoller, roller_options_string.c_str(), LV_ROLLER_MODE_NORMAL);
        }

        lv_timer_handler();
        sleep_ms(5);
    }
}
