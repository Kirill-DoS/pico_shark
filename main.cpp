#include "lvgl/src/widgets/lv_roller.h"
#include "setup.h"
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <stdio.h>
#include <string>
#include <vector>

#include "src/include/temp_press.h"

#include <stdint.h>
#include "lwip/apps/sntp.h"
#include <time.h>
#include <sys/time.h>

#define timezone 3

extern "C" void set_time_bootstrap(uint32_t sec) {
    struct timeval tv = { .tv_sec = (time_t)sec, .tv_usec = 0 };
    settimeofday(&tv, NULL); // Устанавливаем системное время в Pico SDK
    printf("NTP Time synchronized successfully!\n");
}

const char *ssid = "A1-113";
const char *psw = "99981270";

float hum = 0, temp = 0, press = 0;

int main() {
    stdio_init_all();
    sleep_ms(2000);

    // lvgl and display init
    setup_all();
    printf("Setup complete\n");

    Sensor sens(0, 1);
    sens.aht20_init();
    sens.bmp280_init();

    // wifi init
    cyw43_arch_init();
    printf("WiFi init\n");
    // make statin mode
    cyw43_arch_enable_sta_mode();
    //connect to WiFi
    if(cyw43_arch_wifi_connect_timeout_ms(ssid, psw, CYW43_AUTH_WPA2_AES_PSK, 15000) == 0){
        printf("WiFi connected\n");

        //init sntp
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_init();
    }else{
        printf("WiFI connection failed\n");
    }

    // Переменная для хранения времени следующего опроса датчиков
    absolute_time_t next_sensor_update = get_absolute_time();

    while (true) {
        // 1. Обновление тиков и обработчика LVGL (каждые 5 мс)
        lv_tick_inc(5);
        lv_timer_handler();

        // 2. Обработка времени
        time_t now = time(NULL);
        struct tm *timeinfo = localtime(&now);
        timeinfo->tm_hour = (timeinfo->tm_hour + timezone) % 24;

        char time_str[32];
        snprintf(time_str, sizeof(time_str), "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);

        // 3. Опрос Wi-Fi
        cyw43_arch_poll();

        if(wifi_scan_in_progress && !cyw43_wifi_scan_active(&cyw43_state)){
            wifi_scan_in_progress = false;
            wifi_scan_data_ready = true;
        }

        // 4. Безопасная зона для работы с LVGL и lwIP
        cyw43_arch_lwip_begin();

        // Обновляем время на экране
        lv_label_set_text(ui_TimeLabel, time_str);

        // Обновляем список сетей, если готов
        if(wifi_scan_data_ready){
            wifi_scan_data_ready = false;
            if(!roller_options_string.empty()){
                lv_roller_set_options(ui_NetListRoller, roller_options_string.c_str(), LV_ROLLER_MODE_NORMAL);
            }else{
                lv_roller_set_options(ui_NetListRoller, "No networks found", LV_ROLLER_MODE_NORMAL);
            }
        }

        // НЕБЛОКИРУЮЩИЙ ОПРОС ДАТЧИКОВ (Раз в 2000 мс)
        if (absolute_time_diff_us(get_absolute_time(), next_sensor_update) < 0) {
            printf("Update sensors\n");
            sens.aht20_read(&hum, &temp);
            sens.bmp280_read(&press);

            printf("Value: tepm: %.1f, hum: %.1f, press: %.0f", temp, hum, press);

            // ИСПРАВЛЕНО: Добавлены точки для корректного форматирования float
            lv_label_set_text_fmt(ui_TempValLabel, "%.1f", temp);
            lv_label_set_text_fmt(ui_HumValLabel, "%.1f", hum);       // Теперь выведет, например, 45.2
            lv_label_set_text_fmt(ui_PressValLabel, "%.0f", press);   // Для давления лучше %.0f (без знаков после запятой)

            // Планируем следующий опрос через 2000 мс
            next_sensor_update = make_timeout_time_ms(2000);
        }

        lv_timer_handler();
        cyw43_arch_lwip_end();

        sleep_ms(5);
    }
}
