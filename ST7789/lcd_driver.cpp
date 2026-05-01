#include "lcd_driver.h"
#include "hardware/gpio.h"
#include "st7789_lcd.pio.h"
#include "../fonts/simple_font_8x12/simple_font.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SERIAL_CLK_DIV 1.f


LCD::LCD(PIO pio, uint sm) : pio(pio), sm(sm), cursor_x(0), cursor_y(0) {
    // Инициализация пинов
    gpio_init(PIN_CS);
    gpio_init(PIN_DC);
    gpio_init(PIN_RESET);

    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_set_dir(PIN_RESET, GPIO_OUT);

    // Сброс дисплея
    gpio_put(PIN_CS, 1);
    gpio_put(PIN_RESET, 1);
    sleep_ms(10);
    gpio_put(PIN_RESET, 0);
    sleep_ms(10);
    gpio_put(PIN_RESET, 1);
    sleep_ms(120);

    // Добавление PIO программы
    uint offset = pio_add_program(pio, &st7789_lcd_program);
    st7789_lcd_program_init(pio, sm, offset, PIN_DIN, PIN_CLK, SERIAL_CLK_DIV);

    // Последовательность инициализации
    const uint8_t init_cmds[] = {
        1, 20,  ST7789_SWRESET,      // Сброс
        1, 10,  ST7789_SLPOUT,       // Выход из спящего режима
        2, 2,   ST7789_COLMOD, 0x55, // 16-битный цвет
        2, 0,   ST7789_MADCTL, 0x60, // Настройка адресации
        5, 0,   ST7789_CASET, 0x00, 0x00, SCREEN_WIDTH >> 8, SCREEN_WIDTH & 0xFF,  // Установка столбцов
        5, 0,   ST7789_RASET, 0x00, 0x00, SCREEN_HEIGHT >> 8, SCREEN_HEIGHT & 0xFF, // Установка строк
        1, 2,   ST7789_INVON,        // Инверсия включена
        1, 2,   0x13,                // Нормальный режим
        1, 2,   ST7789_DISPON,       // Включение дисплея
        0                           // Конец
    };

    // Отправка команд инициализации
    const uint8_t *cmd = init_cmds;
    while (*cmd) {
        lcd_write_cmd(pio, sm, cmd + 2, *cmd);
        sleep_ms(*(cmd + 1) * 5);
        cmd += *cmd + 2;
    }
    last_draw_text[0] = '\0';
}

void LCD::lcd_start_pixels() {
    uint8_t cmd = ST7789_RAMWR;
    lcd_write_cmd(pio, sm, &cmd, 1);
    lcd_set_dc_cs(1, 0);
}

void LCD::lcd_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t caset_cmd[] = {
        ST7789_CASET,
        (uint8_t)(x0 >> 8), (uint8_t)(x0 & 0xFF),
        (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF)
    };

    uint8_t raset_cmd[] = {
        ST7789_RASET,
        (uint8_t)(y0 >> 8), (uint8_t)(y0 & 0xFF),
        (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF)
    };

    lcd_write_cmd(pio, sm, caset_cmd, 5);
    lcd_write_cmd(pio, sm, raset_cmd, 5);
}

void LCD::lcd_fill_screen(uint16_t color) {
    lcd_set_address_window(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
    lcd_start_pixels();

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    for (uint32_t i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        st7789_lcd_put(pio, sm, hi);
        st7789_lcd_put(pio, sm, lo);
    }

    st7789_lcd_wait_idle(pio, sm);
    lcd_set_dc_cs(1, 1);
}

void LCD::lcd_clear_screen(uint16_t color) {
    lcd_fill_screen(color);
}

void LCD::lcd_set_cursor(uint16_t x, uint16_t y) {
    cursor_x = x;
    cursor_y = y;
}

int LCD::lcd_get_cursor_x(){
    return cursor_x;
}

int LCD::lcd_get_cursor_y(){
    return cursor_y;
}

void LCD::lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return;

    lcd_set_address_window(x, y, x, y);
    lcd_start_pixels();

    st7789_lcd_put(pio, sm, color >> 8);
    st7789_lcd_put(pio, sm, color & 0xFF);

    st7789_lcd_wait_idle(pio, sm);
    lcd_set_dc_cs(1, 1);
}

void LCD::lcd_draw_cursor_string(const char *str, uint16_t color, uint16_t bg_color) {
    if (last_draw_text[0] != '\0') {
        int old_width = strlen(last_draw_text) * FONT_WIDTH + FONT_SPACING;
        for (int i = 0; i < old_width; i++) {
            for (int j = 0; j < FONT_HEIGHT; j++) {
                lcd_draw_pixel(cursor_x + i, cursor_y + j, bg_color);
            }
        }
    }
    lcd_draw_string_extended(cursor_x, cursor_y, str, color, bg_color);
    strcpy(last_draw_text, str);
    cursor_x += strlen(str) *FONT_WIDTH + FONT_SPACING;
}

void LCD::draw_char_extended(int x, int y, char c, uint16_t color, uint16_t bg_color) {
    int index = font_get_index(c);

    for (int row = 0; row < 12; row++) {
        uint8_t line = digits_font[index][row];
        for (int col = 0; col < 8; col++) {
            if (line & (0x80 >> col)) {
                lcd_draw_pixel(x + col, y + row, color);
            } else if (bg_color != 0xFFFF) {
                lcd_draw_pixel(x + col, y + row, bg_color);
            }
        }
    }
}

void LCD::lcd_draw_string_extended(int x, int y, const char* str, uint16_t color, uint16_t bg_color) {
    int cx = x;
    while (*str) {
        if (*str == ' ') {
            cx += 9;
        } else if (*str == '\n') {
            y += 13;
            cx = x;
        } else {
            draw_char_extended(cx, y, *str, color, bg_color);
            cx += 9;
        }
        str++;
    }
}

void LCD::lcd_draw_fill_rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
    for (uint16_t w = x0; w < x1; w++) {
        for (uint16_t h = y0; h < y1; h++) {
            lcd_draw_pixel(w, h, color);
        }
    }
}

void LCD::lcd_draw_rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color){
    lcd_draw_horizontal_line(x0, x1,y0,color);
    lcd_draw_horizontal_line(x0, x1, y1, color);
    lcd_draw_vertical_line(y0, y1, x0, color);
    lcd_draw_vertical_line(y0, y1, x1, color);
}

void LCD::lcd_draw_horizontal_line(int x1, int x2, int y, uint16_t color) {
    for (int i = x1; i <= x2; i++) {
        lcd_draw_pixel(i, y, color);
    }
}

void LCD::lcd_draw_vertical_line(int y1, int y2, int x, uint16_t color) {
    for (int i = y1; i <= y2; i++) {
        lcd_draw_pixel(x, i, color);
    }
}

void LCD::lcd_draw_circle(int x0, int y0, int radius, uint16_t color) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    while (x < y) {
        lcd_draw_pixel(x0 + x, y0 + y, color);
        lcd_draw_pixel(x0 - x, y0 + y, color);
        lcd_draw_pixel(x0 + x, y0 - y, color);
        lcd_draw_pixel(x0 - x, y0 - y, color);
        lcd_draw_pixel(x0 + y, y0 + x, color);
        lcd_draw_pixel(x0 - y, y0 + x, color);
        lcd_draw_pixel(x0 + y, y0 - x, color);
        lcd_draw_pixel(x0 - y, y0 - x, color);

        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void LCD::lcd_draw_fill_circle(int x0, int y0, int radius, uint16_t color) {
    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    while (x <= y) {
        lcd_draw_horizontal_line(x0 - x, x0 + x, y0 + y, color);
        lcd_draw_horizontal_line(x0 - x, x0 + x, y0 - y, color);
        lcd_draw_horizontal_line(x0 - y, x0 + y, y0 + x, color);
        lcd_draw_horizontal_line(x0 - y, x0 + y, y0 - x, color);

        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}
