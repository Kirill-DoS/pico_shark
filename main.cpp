#include "lvgl/src/widgets/lv_label.h"
#include "lvgl/src/widgets/lv_roller.h"
#include "pins.h"
#include "setup.h"
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <stdio.h>
#include <string>
#include <vector>

#include "src/temp_press/temp_press.h"
#include "src/pn532/pn532.h"

#include <stdint.h>
#include "lwip/apps/sntp.h"
#include "ui/screens/ui_Screen1.h"
#include <time.h>
#include <sys/time.h>

static const int TIMEZONE_OFFSET_HOURS = 3;

extern "C" void set_time_bootstrap(uint32_t sec) {
    struct timeval tv;
    tv.tv_sec = (time_t)sec;
    tv.tv_usec = 0;

    settimeofday(&tv, NULL);

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

    // wifi init
    if (cyw43_arch_init() == 0) {
        wifi_chip_active = true;

        printf("WiFi init OK\n");

        cyw43_arch_enable_sta_mode();

        if (cyw43_arch_wifi_connect_timeout_ms(
            ssid,
            psw,
            CYW43_AUTH_WPA2_AES_PSK,
            8000
        ) == 0) {

            printf("WiFi connected\n");

            //sntp_setservername(0, "pool.ntp.org");
            //sntp_set_time_sync_notification_cb(set_time_bootstrap);
            sntp_setoperatingmode(SNTP_OPMODE_POLL);
            sntp_init();

        } else {
            printf("WiFi connection failed\n");
        }

    } else {
        wifi_chip_active = false;
        printf("WiFi init failed\n");
    }

    // init AHT and BMP280 sensors on i2c0
    // Sensor sens(SDA_I2C, SCL_I2C);
    // sens.aht20_init();
    // sens.bmp280_init();

    PN532 nfc(I2C_PORT, SDA_I2C, SCL_I2C);

    if(nfc.begin()){
                printf("Инициализация успешна\n");
            }else{
                printf("Инициализация провалилась\n");
            }

            uint32_t version = 0;
            if (!nfc.getFirmwareVersion(version)) {
                printf("PN532 не найден! Проверьте подключение.\\n");
                while (1) sleep_ms(1000);
            }

            printf("Найден чип PN532. Версия прошивки: 0x%08X\\n", version);
            nfc.samConfig(); // Включение SAM (Secure Access Module)

            printf("Ожидание NFC/RFID метки...\\n");




    // Переменная для хранения времени следующего опроса датчиков
    absolute_time_t next_sensor_update = get_absolute_time();

    uint8_t uid[7]; // Массив под UID (может быть до 7 байт для Mifare Ultralight/DESFire)
    uint8_t uidLength = 0;
    while (true) {
        // 1. LVGL tick и обработка
        lv_tick_inc(5);
        lv_timer_handler();

        // Wi-Fi
        if (wifi_chip_active) {
            cyw43_arch_poll();

            if (wifi_scan_in_progress && !cyw43_wifi_scan_active(&cyw43_state)) {
                wifi_scan_in_progress = false;
                wifi_scan_data_ready = true;
            }
        }

        if (wifi_scan_data_ready) {
            wifi_scan_data_ready = false;
            update_wifi_roller_from_scan_results();
        }

        //  Датчики
        // if (absolute_time_diff_us(get_absolute_time(), next_sensor_update) < 0) {
        //     //printf("Update sensors\n");
        //
        //     sens.aht20_read(&hum, &temp);
        //     sens.bmp280_read(&press);
        //
        //     //printf("Value: temp: %.1f, hum: %.1f, press: %.0f\n", temp, hum, press);
        //
        //     lv_label_set_text_fmt(ui_TempValLabel, "%.1f", temp);
        //     lv_label_set_text_fmt(ui_HumValLabel, "%.1f", hum);
        //     lv_label_set_text_fmt(ui_PressValLabel, "%.0f", press);
        //
        //     next_sensor_update = make_timeout_time_ms(2000);
        // }

        //6. Вывод UID
        if (PN532::scan_in_progress) {
            if (nfc.readPassiveTargetID(uid, uidLength)) {
                // Карта найдена — останавливаем сканирование
                PN532::scan_in_progress = false;
                PN532::data_ready = true;

                printf("\n=== КАРТА ПРИНЯТА ===\n");
                printf("UID: ");
                for (uint8_t i = 0; i < uidLength; i++) {
                    printf("0x%02X ", uid[i]);
                }
                printf("\n");

                char uid_str[32];
                int pos = 0;
                for (uint8_t i = 0; i < uidLength && pos < 28; i++) {
                    pos += snprintf(uid_str + pos, sizeof(uid_str) - pos, "%02X ", uid[i]);
                }
                lv_label_set_text(ui_OutputLabel, uid_str);

                sleep_ms(1000); // защита от повторного срабатывания той же карты
            } else {
                printf("card not found\n");
                // НЕ нашли — обязательная пауза перед следующей попыткой
                sleep_ms(200);
                PN532::scan_in_progress = false;
                lv_label_set_text(ui_OutputLabel, "card not detected");
            }
            printf("exit\n");
        }

    }
}
