#pragma once
#include "ST7789/ST7789.h"
#include "lvgl/lvgl.h"
#include "ui/ui.h"

#include "src/enkoder/enkoder.h"
#include "src/pn532/pn532.h"
#include "pico/cyw43_arch.h"

#include <cstdint>
#include <string>
#include <vector>

#define MAX_PANEL 3

// WiFi state
extern volatile bool wifi_chip_active;
extern bool is_scanning_now;
extern volatile bool wifi_scan_in_progress;
extern volatile bool wifi_scan_data_ready;

// Драйвер ввода
extern lv_indev_t * indev_encoder;

// Группы управления
extern lv_group_t * main_menu_group;
extern lv_group_t * rfid_group;
extern lv_group_t * wifi_group;
extern lv_group_t * bt_group;

// Функции инициализации и чтения
void encoder_read_cb(lv_indev_drv_t * drv, lv_indev_data_t * data);
void setup_all(void);

// Обработчики нажатий на элементы интерфейса
void rfid_panel_click_cb(lv_event_t * e);
void wifi_panel_click_cb(lv_event_t * e);
void bt_panel_click_cb(lv_event_t * e);
void back_button_click_cb(lv_event_t * e);
void scan_button_click_cb(lv_event_t * e);
void read_card_button_click_cb(lv_event_t * e);
void emulate_card_button_click_cb(lv_event_t * e);

// for WiFi chip
void update_wifi_roller_from_scan_results(void);
extern std::string roller_options_string;
extern lv_obj_t * ui_NetListRoller;
extern volatile bool wifi_chip_active;
int work_wifi_scan_result_cb(void *env, const cyw43_ev_scan_result_t *result);
