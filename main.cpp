#include "lvgl/src/widgets/lv_roller.h"
#include "setup.h"
#include <cstddef>
#include <cstdint>
#include <stdio.h>
#include <string>
#include <vector>

int main() {
    stdio_init_all();
    sleep_ms(2000);

    //init wifi module
    if(cyw43_arch_init()){
        printf("WiFi init failde\n");
        return -1;
    }
    cyw43_arch_enable_sta_mode();

    setup_all();
    printf("Setup complete\n");


    while (true) {
        lv_tick_inc(5);
        lv_timer_handler();

        cyw43_arch_poll();

        if(wifi_scan_in_progress && !cyw43_wifi_scan_active(&cyw43_state)){
            wifi_scan_in_progress = false;
            wifi_scan_data_ready = true;
        }

        cyw43_arch_lwip_begin();

        if(wifi_scan_data_ready){
            wifi_scan_data_ready = false;
            if(!roller_options_string.empty()){
                lv_roller_set_options(ui_NetListRoller, roller_options_string.c_str(), LV_ROLLER_MODE_NORMAL);
            }else{
                lv_roller_set_options(ui_NetListRoller, "No networks found", LV_ROLLER_MODE_NORMAL);
            }
        }
        lv_timer_handler();
        cyw43_arch_lwip_end();

        sleep_ms(5);
    }
}
