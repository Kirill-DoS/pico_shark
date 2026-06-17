#include "setup.h"
#include "lvgl/src/core/lv_event.h"
#include "lvgl/src/core/lv_group.h"
#include "pico/stdlib.h"
#include "ui/screens/ui_Screen1.h"
#include <stdio.h>

#include <vector>
#include <string>
#include <algorithm>

std::vector<std::string> found_ssids;
std::string roller_options_string;
bool is_scanning_now = false;

volatile bool wifi_scan_in_progress = false;
volatile bool wifi_scan_data_ready = false;

// Железо экрана
static ST7789 display(5, 4, 3, spi0);
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[320 * 20];
static lv_disp_drv_t disp_drv;

// Указатели на устройства ввода и группы
lv_indev_t * indev_encoder = NULL;
lv_group_t * main_menu_group = NULL;
lv_group_t * rfid_group = NULL;
lv_group_t * wifi_group = NULL;

// Переменные жесткого контроля страниц (0 = Home, 1 = RFID, 2 = WiFi)
int current_page = 0;
static int last_counter_value = 0;
static bool in_sub_menu = false;

// Чтение энкодера с защитой от "убегания" интерфейса
void encoder_read_cb(lv_indev_drv_t * drv, lv_indev_data_t * data) {
	int current_counter = counter;
	int diff = current_counter - last_counter_value;
	last_counter_value = current_counter;

	if (in_sub_menu) {
		data->enc_diff = diff; // Внутри подменю работает обычный скролл LVGL
	} else {
		data->enc_diff = 0; // На главном экране скролл LVGL отключен

		if (diff != 0) {
			// КРУГОВАЯ НАВИГАЦИЯ
			if (diff > 0) {
				current_page++;
				if (current_page > MAX_PANEL) current_page = 0; // С WiFi (2) переходим на Home (0)
			} else if (diff < 0) {
				current_page--;
				if (current_page < 0) current_page = MAX_PANEL; // С Home (0) переходим на WiFi (2)
			}

			printf("Switching page to: %d\n", current_page);

			// Сдвигаем карусель
			lv_obj_scroll_to_y(ui_uiMenuCarousel, current_page * 240, LV_ANIM_OFF);

			// Фокусируем нужную панель
			if (current_page == 0) lv_group_focus_obj(ui_uiHomePanel);
			else if (current_page == 1) lv_group_focus_obj(ui_uiRFIDPanel);
			else if (current_page == 2) lv_group_focus_obj(ui_uiWiFiPanel);
		}
	}

	// Состояние кнопки
	if (gpio_get(SW) == 0) {
		data->state = LV_INDEV_STATE_PR;
	} else {
		data->state = LV_INDEV_STATE_REL;
	}
}

static void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
	uint16_t w = (uint16_t)(area->x2 - area->x1 + 1);
	uint16_t h = (uint16_t)(area->y2 - area->y1 + 1);
	display.pushColors(area->x1, area->y1, w, h, (uint16_t*)color_p);
	lv_disp_flush_ready(disp_drv);
}

// Заход в RFID подменю
void rfid_panel_click_cb(lv_event_t * e) {
	printf("Click: Entering RFID sub-menu\n");
	in_sub_menu = true;
	lv_indev_set_group(indev_encoder, rfid_group);
	lv_group_focus_obj(ui_BackButton);
}

// Заход в WiFi подменю
void wifi_panel_click_cb(lv_event_t * e) {
	printf("Click: Entering WiFi sub-menu\n");
	in_sub_menu = true;
	lv_indev_set_group(indev_encoder, wifi_group);
	lv_group_focus_obj(ui_BackButtonWiFi);
}

// Выход из подменю обратно на карусель
void back_button_click_cb(lv_event_t * e) {
	printf("Back to Main Menu Carousel\n");
	in_sub_menu = false;
	lv_indev_set_group(indev_encoder, main_menu_group);

	if (current_page == 1) lv_group_focus_obj(ui_uiRFIDPanel);
	else if (current_page == 2) lv_group_focus_obj(ui_uiWiFiPanel);
}

// Клик по кнопке Scan
void scan_button_click_cb(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED){
		if(!wifi_scan_in_progress){
			wifi_scan_in_progress = true;
			wifi_scan_data_ready = false;

			roller_options_string.clear();
			cyw43_wifi_scan_options_t scan_options = {0};
			cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, work_wifi_scan_result_cb);
		}
	}
	// if (is_scanning_now) return;
 //
	// printf("Scan button triggered!\n");
	// is_scanning_now = true;
 //
	// // Выводим текст ожидания в роллер
	// lv_roller_set_options(ui_NetListRoller, "Scanning...\nPlease wait...", LV_ROLLER_MODE_NORMAL);
	// lv_obj_invalidate(ui_NetListRoller);
 //
	// wifi_scan_network();
}

// Инициализация всего интерфейса
void setup_all(void) {
	display.begin();
	lv_init();

	lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320 * 20);
	lv_disp_drv_init(&disp_drv);
	disp_drv.hor_res = 320;
	disp_drv.ver_res = 240;
	disp_drv.flush_cb = my_disp_flush;
	disp_drv.draw_buf = &draw_buf;
	lv_disp_drv_register(&disp_drv);

	ui_init();
	encoder_init();

	static lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_ENCODER;
	indev_drv.read_cb = encoder_read_cb;
	indev_encoder = lv_indev_drv_register(&indev_drv);

	main_menu_group = lv_group_create();
	rfid_group      = lv_group_create();
	wifi_group      = lv_group_create();

	// Неоновые рамки фокуса
	static lv_style_t style_focused;
	lv_style_init(&style_focused);
	lv_style_set_border_width(&style_focused, 3);
	lv_style_set_border_color(&style_focused, lv_color_hex(0x00D9FF));
	lv_style_set_border_opa(&style_focused, LV_OPA_COVER);

	lv_obj_add_style(ui_uiHomePanel, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_uiRFIDPanel, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_uiWiFiPanel, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_BackButton, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_SearchButton, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_BackButtonWiFi, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_ScanWiFiBut, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_NetListRoller, &style_focused, LV_STATE_FOCUSED);

	// Навешивание элементов на группы
	lv_group_add_obj(main_menu_group, ui_uiHomePanel);
	lv_group_add_obj(main_menu_group, ui_uiRFIDPanel);
	lv_group_add_obj(main_menu_group, ui_uiWiFiPanel);
	lv_obj_add_event_cb(ui_uiRFIDPanel, rfid_panel_click_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(ui_uiWiFiPanel, wifi_panel_click_cb, LV_EVENT_CLICKED, NULL);

	lv_group_add_obj(rfid_group, ui_BackButton);
	lv_group_add_obj(rfid_group, ui_SearchButton);
	lv_obj_add_event_cb(ui_BackButton, back_button_click_cb, LV_EVENT_CLICKED, NULL);

	lv_group_add_obj(wifi_group, ui_BackButtonWiFi);
	lv_group_add_obj(wifi_group, ui_ScanWiFiBut);
	lv_group_add_obj(wifi_group, ui_NetListRoller);
	lv_obj_add_event_cb(ui_BackButtonWiFi, back_button_click_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(ui_ScanWiFiBut, scan_button_click_cb, LV_EVENT_CLICKED, NULL);

	// Стартовая геометрия
	lv_obj_set_scroll_dir(ui_uiMenuCarousel, LV_DIR_NONE);
	lv_obj_set_style_anim_time(ui_uiMenuCarousel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_update_layout(ui_uiMenuCarousel);
	lv_obj_scroll_to_y(ui_uiMenuCarousel, 0, LV_ANIM_OFF);

	lv_indev_set_group(indev_encoder, main_menu_group);
	lv_group_focus_obj(ui_uiHomePanel);

	current_page = 0;
	in_sub_menu = false;
	is_scanning_now = false;
}

// Обработчик результатов Wi-Fi
// int scan_result_cb(void *env, const cyw43_ev_scan_result_t *result) {
// 	if (result) {
// 		if (result->ssid_len > 32 || result->ssid_len == 0) return 0;
//
// 		char safe_ssid[33];
// 		memset(safe_ssid, 0, sizeof(safe_ssid));
// 		memcpy(safe_ssid, result->ssid, result->ssid_len);
// 		std::string name(safe_ssid);
// 		if (name.empty()) name = "[Hidden]";
//
// 		if (std::find(found_ssids.begin(), found_ssids.end(), name) == found_ssids.end()) {
// 			found_ssids.push_back(name);
// 			printf("Found Wi-Fi: %s\n", name.c_str());
// 		}
// 	} else {
// 		printf("Scan finished. Formatting options...\n");
// 		roller_options_string = "";
// 		for (const auto& s : found_ssids) {
// 			roller_options_string += s + "\n";
// 		}
// 		if (!roller_options_string.empty()) {
// 			roller_options_string.pop_back();
// 		} else {
// 			roller_options_string = "No networks found";
// 		}
// 	}
// 	return 0;
// }

int work_wifi_scan_result_cb(void *env, const cyw43_ev_scan_result_t *result){
	if(!result) return 0;

	std::string ssid_str(reinterpret_cast<const char*>(result->ssid));

	if(ssid_str.empty()) return 0;

	if(roller_options_string.find(ssid_str) == std::string::npos){
		if(!roller_options_string.empty()){
			roller_options_string += "\n";
		}

		roller_options_string +=ssid_str;
	}
	return 0;
}

// void wifi_scan_network() {
// 	printf("Starting Wi-Fi scan process...\n");
// 	found_ssids.clear();
//
// 	int init_res = cyw43_arch_init();
// 	if (init_res != 0) {
// 		printf("Failed to init CYW43: %d\n", init_res);
// 		is_scanning_now = false;
// 		return;
// 	}
//
// 	cyw43_arch_enable_sta_mode();
// 	cyw43_wifi_scan_options_t scan_options = {0};
// 	int err = cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, scan_result_cb);
//
// 	if (err < 0) {
// 		printf("Scan trigger failed: %d. Cleaning up...\n", err);
// 		cyw43_arch_deinit();
// 		is_scanning_now = false;
// 	}
// }
