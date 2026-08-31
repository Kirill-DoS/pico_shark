#include "setup.h"
#include "lvgl/src/core/lv_event.h"
#include "lvgl/src/core/lv_group.h"
#include "pico/stdlib.h"
#include "pins.h"
#include "src/pn532/pn532.h"
#include "ui/screens/ui_Screen1.h"
#include <stdio.h>
#include "pins.h"
#include <vector>
#include <string>
#include <algorithm>

std::vector<std::string> found_ssids;
std::string roller_options_string;
bool is_scanning_now = false;

volatile bool wifi_scan_in_progress = false;
volatile bool wifi_scan_data_ready = false;
volatile bool wifi_chip_active = false;

volatile bool pn_read_cadr_in_progress = false;
volatile bool pn_card_uid_ready = false;
std::vector<uint8_t> uid;
volatile bool pn_emulate_card_in_progress = false;
volatile bool is_pn_emulate_btn_clicked = false;

// Железо экрана
static ST7789 display(CS_DISP, DC_DISP, RST_DISP, SPI_PORT);
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[320 * 20];
static lv_disp_drv_t disp_drv;

// Указатели на устройства ввода и группы
lv_indev_t * indev_encoder = NULL;
lv_group_t * main_menu_group = NULL;
lv_group_t * rfid_group = NULL;
lv_group_t * wifi_group = NULL; 
lv_group_t * bt_group = NULL;

// Переменные жесткого контроля страниц (0 = Home, 1 = RFID, 2 = WiFi, 3 = BT)
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
			else if (current_page == 3) lv_group_focus_obj(ui_uiBTPanel);
		}
	}

	// Состояние кнопки
	if (gpio_get(SW_ENK) == 0) {
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

void bt_panel_click_cb(lv_event_t * e) {
	printf("Click: Entering BT sub-menu\n");
	in_sub_menu = true;
	lv_indev_set_group(indev_encoder, bt_group);
	lv_group_focus_obj(ui_BLEBackBtn);
}

// Выход из подменю обратно на карусель
void back_button_click_cb(lv_event_t * e) {
	printf("Back to Main Menu Carousel\n");
	in_sub_menu = false;
	lv_indev_set_group(indev_encoder, main_menu_group);

	if (current_page == 1) lv_group_focus_obj(ui_uiRFIDPanel);
	else if (current_page == 2) lv_group_focus_obj(ui_uiWiFiPanel);
	else if (current_page == 3) lv_group_focus_obj(ui_uiBTPanel);
}

// Клик по кнопке Scan
void scan_button_click_cb(lv_event_t * e) {
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

	if (!wifi_chip_active) {
		lv_roller_set_options(ui_NetListRoller, "WiFi not ready", LV_ROLLER_MODE_NORMAL);
		return;
	}

	if (!wifi_scan_in_progress) {
		wifi_scan_in_progress = true;
		wifi_scan_data_ready = false;

		found_ssids.clear();
		roller_options_string.clear();

		lv_roller_set_options(ui_NetListRoller, "Scanning...", LV_ROLLER_MODE_NORMAL);

		cyw43_wifi_scan_options_t scan_options = {0};

		int scan_result = cyw43_wifi_scan(
			&cyw43_state,
			&scan_options,
			NULL,
			work_wifi_scan_result_cb
		);

		if (scan_result != 0) {
			wifi_scan_in_progress = false;
			lv_roller_set_options(ui_NetListRoller, "Scan error", LV_ROLLER_MODE_NORMAL);
		}
	}
}

int work_wifi_scan_result_cb(void *env, const cyw43_ev_scan_result_t *result) {
	if (!result) return 0;

	if (result->ssid_len == 0) return 0;

	std::string ssid_str(
		reinterpret_cast<const char*>(result->ssid),
						 result->ssid_len
	);

	if (ssid_str.empty()) return 0;

	if (std::find(found_ssids.begin(), found_ssids.end(), ssid_str) == found_ssids.end()) {
		found_ssids.push_back(ssid_str);
	}

	return 0;
}

void update_wifi_roller_from_scan_results(void) {
	std::sort(found_ssids.begin(), found_ssids.end());

	roller_options_string.clear();

	for (size_t i = 0; i < found_ssids.size(); i++) {
		if (i > 0) {
			roller_options_string += "\n";
		}

		roller_options_string += found_ssids[i];
	}

	if (!roller_options_string.empty()) {
		lv_roller_set_options(
			ui_NetListRoller,
			roller_options_string.c_str(),
							  LV_ROLLER_MODE_NORMAL
		);
	} else {
		lv_roller_set_options(
			ui_NetListRoller,
			"No networks found",
			LV_ROLLER_MODE_NORMAL
		);
	}
}

void read_card_button_click_cb(lv_event_t * e) {
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

	printf("Read card button clicked\n");
	lv_label_set_text(ui_RFIDTagLabel, "Reading...");

	PN532::scan_in_progress = true;
	PN532::data_ready = false;
}

void emulate_card_button_click_cb(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED){
		//pn_emulate_card_in_progress = true;
		is_pn_emulate_btn_clicked = true;
	}
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
	bt_group 		= lv_group_create();

	// Неоновые рамки фокуса
	static lv_style_t style_focused;
	lv_style_init(&style_focused);
	lv_style_set_border_width(&style_focused, 3);
	lv_style_set_border_color(&style_focused, lv_color_hex(0x00D9FF));
	lv_style_set_border_opa(&style_focused, LV_OPA_COVER);

	// add style for panels
	lv_obj_add_style(ui_uiHomePanel, &style_focused, LV_STATE_FOCUSED); // home panel
	lv_obj_add_style(ui_uiRFIDPanel, &style_focused, LV_STATE_FOCUSED); // RFID panel
	lv_obj_add_style(ui_uiWiFiPanel, &style_focused, LV_STATE_FOCUSED); // WiFi panel
	lv_obj_add_style(ui_uiBTPanel, &style_focused, LV_STATE_FOCUSED);   // BT panel
	// add style for UI elements
	// rfid panel
	lv_obj_add_style(ui_BackButton, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_ReadBtn, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_EmulateBtn, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_TagSelectRoller, &style_focused, LV_STATE_FOCUSED);
	// wifi panel
	lv_obj_add_style(ui_BackButtonWiFi, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_ScanWiFiBut, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_NetListRoller, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_EvilTwinBtn, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_PortScanBtn, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_StartBtn, &style_focused, LV_STATE_FOCUSED);
	// bt panel
	lv_obj_add_style(ui_BLEBackBtn, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_BLEDataRoller, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_BLEFloodBtn, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_BTHIDBtn, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_BTScan, &style_focused, LV_STATE_FOCUSED);

	// Навешивание элементов на группы
	lv_group_add_obj(main_menu_group, ui_uiHomePanel);
	lv_group_add_obj(main_menu_group, ui_uiRFIDPanel);
	lv_group_add_obj(main_menu_group, ui_uiWiFiPanel);
	lv_group_add_obj(main_menu_group, ui_uiBTPanel);
	lv_obj_add_event_cb(ui_uiRFIDPanel, rfid_panel_click_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(ui_uiWiFiPanel, wifi_panel_click_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(ui_uiBTPanel, bt_panel_click_cb, LV_EVENT_CLICKED, NULL);

	lv_group_add_obj(rfid_group, ui_BackButton);
	lv_group_add_obj(rfid_group, ui_ReadBtn);
	lv_group_add_obj(rfid_group, ui_EmulateBtn);
	lv_group_add_obj(rfid_group, ui_TagSelectRoller);
	lv_obj_add_event_cb(ui_BackButton, back_button_click_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(ui_ReadBtn, read_card_button_click_cb, LV_EVENT_CLICKED, NULL);

	lv_group_add_obj(wifi_group, ui_BackButtonWiFi);
	lv_group_add_obj(wifi_group, ui_ScanWiFiBut);
	lv_group_add_obj(wifi_group, ui_NetListRoller);
	lv_group_add_obj(wifi_group, ui_EvilTwinBtn);
	lv_group_add_obj(wifi_group, ui_PortScanBtn);
	lv_obj_add_event_cb(ui_BackButtonWiFi, back_button_click_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(ui_ScanWiFiBut, scan_button_click_cb, LV_EVENT_CLICKED, NULL);

	lv_group_add_obj(bt_group, ui_BLEBackBtn);
	lv_group_add_obj(bt_group, ui_BLEDataRoller);
	lv_group_add_obj(bt_group, ui_BLEFloodBtn);
	lv_group_add_obj(bt_group, ui_BTHIDBtn);
	lv_group_add_obj(bt_group, ui_BTScan);
	lv_obj_add_event_cb(ui_BLEBackBtn, back_button_click_cb, LV_EVENT_CLICKED, NULL);
	
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
