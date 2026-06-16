#pragma once
#include "ST7789/ST7789.h"
#include "lvgl/lvgl.h"
#include "ui/ui.h"
#include "src/include/enkoder.h"
#include "pico/cyw43_arch.h"

// Драйвер ввода
extern lv_indev_t * indev_encoder;

// Группы управления
extern lv_group_t * main_menu_group;
extern lv_group_t * rfid_group;
extern lv_group_t * wifi_group;

// Функции инициализации и чтения
void encoder_read_cb(lv_indev_drv_t * drv, lv_indev_data_t * data);
void setup_all(void);

// Обработчики нажатий на элементы интерфейса
void rfid_panel_click_cb(lv_event_t * e);
void wifi_panel_click_cb(lv_event_t * e);
void back_button_click_cb(lv_event_t * e);
void scan_button_click_cb(lv_event_t * e);

// for WiFi chip
extern volatile bool wifi_scan_finished;
extern std::string roller_options_string;
extern lv_obj_t * ui_NetListRoller;

void wifi_scan_network(void);
int scan_result_cb(void *env, const cyw43_ev_scan_result_t *result);

