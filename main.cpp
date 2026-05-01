#include "lvgl/src/core/lv_obj.h"
#include "lvgl/src/hal/lv_hal_tick.h"
//#include "lvgl/src/misc/lv_anim.h"
//#include "lvgl/src/widgets/lv_roller.h"
#include "pico/stdlib.h"
#include "lvgl/lvgl.h"
//#include "ui/screens/ui_Screen1.h"
#include "ui/ui.h"

// driver
#include "ST7789/ST7789.h"

#include <cstddef>
#include <cstdint>
#include <stdio.h>

ST7789 display(5, 4, 3, spi0); // Твои пины

void debug_fill_screen() {
    display.fillScreen(0xF800); // Красный (в формате 565)
    sleep_ms(1000);
    display.fillScreen(0x07E0); // Зеленый
    sleep_ms(1000);
}

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[320 * 20]; // Подставь свою ширину
static lv_disp_drv_t disp_drv;

static void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    uint16_t w = (uint16_t)(area->x2 - area->x1 + 1);
    uint16_t h = (uint16_t)(area->y2 - area->y1 + 1);
    display.pushColors(area->x1, area->y1, w, h, (uint16_t*)color_p);
    lv_disp_flush_ready(disp_drv);
}

int main() {
    stdio_init_all();
    sleep_ms(2000); // Даем время на инициализацию USB

    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);

    display.begin();

    gpio_put(LED_PIN, 0);

    printf("Firmware started!\n");

    lv_init();
    printf("LVGL init\n");
    void * ptr = lv_mem_alloc(100);
    if(ptr) printf("Memory OK\n");
    else printf("Memory FAIL\n");

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320 * 20);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;

     lv_disp_drv_register(&disp_drv);

    // add_repeating_timer_ms(1, [](repeating_timer_t *rt) -> bool {
    //     lv_tick_inc(1);
    //     //printf("Tick\n");
    //     return true;
    // }, NULL, NULL);

    ui_init();

    //test
    // printf("Create screem\n");
    // lv_obj_t * test_obj = lv_obj_create(lv_scr_act());
    //
    // if(test_obj == NULL){
    //     printf("ERROR: :VGL could not create obhect");
    // }else {
    //     printf("SUCCESS\n");
    // }
    //
    // printf("Screen create succes\n");
    // printf("befor while loop\n");

    while (true) {
        lv_tick_inc(5);

        printf("in while loop\n");
        lv_timer_handler();
        sleep_ms(5);
    }
}
