#include "setup.h"
#include "pico/stdlib.h"
#include <stdio.h>

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

// Переменные ручного счетчика страниц (0 = Home, 1 = RFID, 2 = WiFi)
int current_page = 0;
static int last_counter_value = 0;
static bool in_sub_menu = false; // Флаг нахождения внутри подменю

// Чтение и ручная обработка навигации
void encoder_read_cb(lv_indev_drv_t * drv, lv_indev_data_t * data) {
	int current_counter = counter;
	int diff = current_counter - last_counter_value;
	last_counter_value = current_counter;

	// Всегда передаем кручение ручки в LVGL (для внутренних меню)
	data->enc_diff = diff;

	// Опрашиваем физическое состояние кнопки прямо сейчас (true = кнопка зажата)
	bool button_is_physically_pressed = (gpio_get(SW) == 0);

	// Статическая переменная, которая помнит, была ли зажата кнопка в ПРЕДЫДУЩЕМ цикле опроса
	static bool button_was_pressed_last_time = false;

	// 1. НАВИГАЦИЯ ПО ГЛАВНОЙ КАРУСЕЛИ (работает только если мы на главном экране)
	if (!in_sub_menu && diff != 0) {
		current_page += diff;
		if (current_page < 0) current_page = 0;
		if (current_page > 2) current_page = 2;

		printf("Page Changed! Current Index: %d\n", current_page);

		if (current_page == 0)      lv_obj_scroll_to_y(ui_uiMenuCarousel, 0, LV_ANIM_OFF);
		else if (current_page == 1) lv_obj_scroll_to_y(ui_uiMenuCarousel, 240, LV_ANIM_OFF);
		else if (current_page == 2) lv_obj_scroll_to_y(ui_uiMenuCarousel, 480, LV_ANIM_OFF);
	}

	// 2. УМНАЯ ОБРАБОТКА КНОПКИ (РАЗДЕЛЕНИЕ РЕЖИМОВ)
	if (!in_sub_menu) {
		// --- МЫ НА ГЛАВНОМ ЭКРАНЕ ---

		// Провал в подменю делаем строго ПО ОТПУСКАНИЮ кнопки (Клик)
		// Условие: в прошлый раз кнопка была зажата, а сейчас её физически ОТПУСТИЛИ
		if (button_was_pressed_last_time && !button_is_physically_pressed) {

			if (current_page == 1) {
				printf("Action: Failsafe Enter to RFID panel (On Release)\n");
				in_sub_menu = true;

				// Переключаем энкодер на подменю RFID
				lv_indev_set_group(indev_encoder, rfid_group);
				lv_group_focus_obj(ui_BackButton);

				// Сбрасываем флаг прерывания, чтобы не накапливался
				button_pressed = false;

				// ВАЖНО: Принудительно говорим LVGL, что кнопка сейчас ОТПУЩЕНА,
				// чтобы он случайно не кликнул по кнопке Back в новом меню!
				data->state = LV_INDEV_STATE_REL;
				button_was_pressed_last_time = false;
				return;
			}
			else if (current_page == 2) {
				printf("Action: Failsafe Enter to WiFi panel (On Release)\n");
				in_sub_menu = true;

				lv_indev_set_group(indev_encoder, wifi_group);
				lv_group_focus_obj(ui_BackButtonWiFi);

				button_pressed = false;
				data->state = LV_INDEV_STATE_REL;
				button_was_pressed_last_time = false;
				return;
			}
		}

		// Если мы просто держим кнопку на главном экране — ничего не делаем в LVGL
		data->state = LV_INDEV_STATE_REL;

	} else {
		// --- МЫ УЖЕ ВНУТРИ ПОДМЕНЮ (RFID или WiFi) ---

		// Здесь мы полностью отдаем кнопку под контроль штатного механизма LVGL.
		// Если кнопка физически зажата — шлем STATE_PR (Pressed), если отпущена — STATE_REL (Released)
		if (button_is_physically_pressed) {
			data->state = LV_INDEV_STATE_PR;
		} else {
			data->state = LV_INDEV_STATE_REL;
		}

		// Очищаем фоновый флаг прерывания, чтобы он не выстрелил потом
		button_pressed = false;
	}

	// Запоминаем текущее физическое состояние кнопки для следующего кадра (раз в 15-30мс)
	button_was_pressed_last_time = button_is_physically_pressed;
}

static void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
	uint16_t w = (uint16_t)(area->x2 - area->x1 + 1);
	uint16_t h = (uint16_t)(area->y2 - area->y1 + 1);
	display.pushColors(area->x1, area->y1, w, h, (uint16_t*)color_p);
	lv_disp_flush_ready(disp_drv);
}

// Универсальный выход из подменю обратно на вертикальную карусель страниц
void back_button_click_cb(lv_event_t * e) {
	printf("Back to Main Carousel via Button Click!\n");

	in_sub_menu = false; // Возвращаем управление нашему счетчику страниц

	// Переключаем энкодер обратно на глобальную группу
	lv_indev_set_group(indev_encoder, main_menu_group);
}

void search_button_click_cb(lv_event_t * e) {
	printf("Search button clicked! Starting Wi-Fi Scan...\n");

	// Вызываем вашу готовую функцию сканирования сети
	wifi_scan_network();
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

	// Инициализация автокода SquareLine и железного энкодера
	ui_init();
	encoder_init();

	// Настройка устройства ввода в LVGL
	static lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_ENCODER;
	indev_drv.read_cb = encoder_read_cb;
	indev_encoder = lv_indev_drv_register(&indev_drv);

	// Создаем группы управления
	main_menu_group = lv_group_create();
	rfid_group      = lv_group_create();
	wifi_group      = lv_group_create();

	// Добавляем саму карусель в главную группу для поддержания структуры опроса
	lv_group_add_obj(main_menu_group, ui_uiMenuCarousel);

	// --- Настройка стилей обводки контура (Неоновая рамка фокуса для кнопок) ---
	static lv_style_t style_focused;
	lv_style_init(&style_focused);
	lv_style_set_border_width(&style_focused, 3); // 3 пикселя толщина
	lv_style_set_border_color(&style_focused, lv_color_hex(0x00D9FF)); // Голубой неон
	lv_style_set_border_opa(&style_focused, LV_OPA_COVER);

	// Применяем обводку к реальным кнопкам Назад
	lv_obj_add_style(ui_BackButton, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_BackButtonWiFi, &style_focused, LV_STATE_FOCUSED);

	// --- Группа подменю RFID ---
	lv_group_add_obj(rfid_group, ui_BackButton);
	lv_group_add_obj(rfid_group, ui_OnButton);    // ДОБАВЬТЕ ЭТУ КНОПКУ (Включение RFID)
	lv_group_add_obj(rfid_group, ui_OffButton);   // ДОБАВЬТЕ ЭТУ КНОПКУ (Выключение RFID)
	lv_group_add_obj(rfid_group, ui_SearchButton);// ДОБАВЬТЕ ЭТУ КНОПКУ (Поиск)

	// Применяем неоновый стиль фокуса ко ВСЕМ кнопкам в меню, чтобы видеть куда крутим
	lv_obj_add_style(ui_BackButton, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_OnButton, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_OffButton, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_SearchButton, &style_focused, LV_STATE_FOCUSED);

	lv_obj_add_event_cb(ui_BackButton, back_button_click_cb, LV_EVENT_CLICKED, NULL);


	// --- Группа подменю WiFi ---
	lv_group_add_obj(wifi_group, ui_BackButtonWiFi);
	lv_group_add_obj(wifi_group, ui_OnButtonWiFi);  // ДОБАВЬТЕ КНОПКУ ВКЛЮЧЕНИЯ WIFI
	lv_group_add_obj(wifi_group, ui_OffButtonWiFi); // ДОБАВЬТЕ КНОПКУ ВЫКЛЮЧЕНИЯ WIFI

	lv_obj_add_style(ui_BackButtonWiFi, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_OnButtonWiFi, &style_focused, LV_STATE_FOCUSED);
	lv_obj_add_style(ui_OffButtonWiFi, &style_focused, LV_STATE_FOCUSED);

	lv_obj_add_event_cb(ui_BackButtonWiFi, back_button_click_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(ui_SearchButton, search_button_click_cb, LV_EVENT_CLICKED, NULL);
	// -------------------------------------------------------------------------
	// НАСТРОЙКА СТАРТА ГЕОМЕТРИИ (ДЛЯ COLUMN ВЕРТИКАЛИ)
	// -------------------------------------------------------------------------
	lv_obj_set_scroll_dir(ui_uiMenuCarousel, LV_DIR_NONE); // Полностью отключаем автоскролл
	lv_obj_set_style_anim_time(ui_uiMenuCarousel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

	// Принудительно обновляем COLUMN-сетку, чтобы рассчитать новые координаты Y
	lv_obj_update_layout(ui_uiMenuCarousel);

	// Сбрасываем вертикальный скролл на 0 (Home Panel сверху)
	lv_obj_scroll_to_y(ui_uiMenuCarousel, 0, LV_ANIM_OFF);

	// Запуск
	current_page = 0;
	in_sub_menu = false;
	button_pressed = false;
	lv_indev_set_group(indev_encoder, main_menu_group);

	printf("Setup complete. Vertical layout synchronized.\n");
}


int scan_result_cb(void *env, const cyw43_ev_scan_result_t *result) {
	if (result) {
		printf("SSID: %-32s | RSSI: %4d | Channel: %3d\n",
			   result->ssid, result->rssi, result->channel);
	}
	return 0;
}

void wifi_scan_network() {
	printf("Initializing Wi-Fi chip...\n");

	// Пытаемся инициализировать
	int init_res = cyw43_arch_init();
	if (init_res != 0) {
		printf("Failed to initialize CYW43: %d\n", init_res);
		return; // Если тут ошибка, дальше идти нет смысла
	}

	cyw43_arch_enable_sta_mode();
	printf("Wi-Fi init success, mode STA enabled.\n");

	// Даем время чипу "проснуться"
	sleep_ms(500);

	while (true) {
		if (!cyw43_wifi_scan_active(&cyw43_state)) {
			cyw43_wifi_scan_options_t scan_options = {0};
			int err = cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, scan_result_cb);

			if (err == 0) {
				printf("Scan started...\n");
			} else {
				printf("Scan error: %d\n", err);
			}
		}
		sleep_ms(10000);
	}
}

