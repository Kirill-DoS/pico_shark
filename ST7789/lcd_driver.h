#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "st7789_lcd.pio.h"
#include <cstdint>
#include <stdint.h>

// Конфигурация пинов
#define PIN_DIN 29
#define PIN_CLK 28
#define PIN_CS 27
#define PIN_DC 10
#define PIN_RESET 15

// Команды ST7789
#define ST7789_SWRESET 0x01
#define ST7789_SLPOUT  0x11
#define ST7789_COLMOD  0x3A
#define ST7789_MADCTL  0x36
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_INVON   0x21
#define ST7789_DISPON  0x29
#define ST7789_RAMWR   0x2C
#define ST7789_RAMRD   0x2E

#define COLOR_BLACK         0x0000
#define COLOR_NAVY          0x000F
#define COLOR_DARKGREEN     0x03E0
#define COLOR_DARKCYAN      0x03EF
#define COLOR_MAROON        0x7800
#define COLOR_PURPLE        0x780F
#define COLOR_OLIVE         0x7BE0
#define COLOR_LIGHTGREY     0xC618
#define COLOR_DARKGREY      0x7BEF
#define COLOR_BLUE          0x001F
#define COLOR_GREEN         0x07E0
#define COLOR_CYAN          0x07FF
#define COLOR_RED           0xF800
#define COLOR_MAGENTA       0xF81F
#define COLOR_YELLOW        0xFFE0
#define COLOR_WHITE         0xFFFF
#define COLOR_ORANGE        0xFD20
#define COLOR_GREENYELLOW   0xAFE5
#define COLOR_PINK          0xF81F

#define FILL_COLOR 0x0000 //0x41A6
// Размеры дисплея
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Размеры шрифта
#define FONT_WIDTH 8
#define FONT_HEIGHT 12

// font array 0 - 9 numbers
static const uint8_t font_digits[10][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}  // 9
};

class LCD {
private:
    PIO pio;
    uint sm;
    uint16_t cursor_x;
    uint16_t cursor_y;
    char last_draw_text[20];

    static inline void lcd_set_dc_cs(bool dc, bool cs) {
        sleep_us(1);
        gpio_put(PIN_DC, dc);
        gpio_put(PIN_CS, cs);
        //gpio_put_masked((1u << PIN_DC) | (1u << PIN_CS), (dc ? 1u << PIN_DC : 0) | (cs ? 1u << PIN_CS : 0));
        sleep_us(1);
    }

    static inline void lcd_write_cmd(PIO pio, uint sm, const uint8_t *cmd, size_t count) {
        st7789_lcd_wait_idle(pio, sm);
        lcd_set_dc_cs(0, 0);
        st7789_lcd_put(pio, sm, *cmd++);

        if (count >= 2) {
            st7789_lcd_wait_idle(pio, sm);
            lcd_set_dc_cs(1, 0);
            for (size_t i = 0; i < count - 1; ++i) {
                st7789_lcd_put(pio, sm, *cmd++);
            }
        }

        st7789_lcd_wait_idle(pio, sm);
        lcd_set_dc_cs(1, 1);
    }

    void lcd_start_pixels();

public:
    // Конструктор
    LCD(PIO pio, uint sm);

    // Заливка всего экрана цветом RGB565
    void lcd_fill_screen(uint16_t color);

    // Установка области рисования
    void lcd_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

    // Рисование одного пикселя
    void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

    // Установка курсора
    void lcd_set_cursor(uint16_t x, uint16_t y);

    // getter
    int lcd_get_cursor_x();
    int lcd_get_cursor_y();

    // Вывод строки от позиции курсора
    void lcd_draw_cursor_string(const char *str, uint16_t color, uint16_t bg_color);

    void draw_char_extended(int x, int y, char c, uint16_t color, uint16_t bg_color);

    // Вывод строки от указанных координат
    void lcd_draw_string_extended(int x, int y, const char* str, uint16_t color, uint16_t bg_color);

    // Очистка экрана
    void lcd_clear_screen(uint16_t color);

    // Рисование прямоугольника
    void lcd_draw_fill_rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

    void lcd_draw_rect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

    // Рисование горизонтальной линии
    void lcd_draw_horizontal_line(int x1, int x2, int y, uint16_t color);

    // Рисование вертикальной линии
    void lcd_draw_vertical_line(int y1, int y2, int x, uint16_t color);

    // Рисование окружности
    void lcd_draw_circle(int x0, int y0, int radius, uint16_t color);

    // Рисование заполненной окружности
    void lcd_draw_fill_circle(int x0, int y0, int radius, uint16_t color);

    // add freindly class for privat methods
    friend class LCD_GFX;

    uint16_t get_width() const { return SCREEN_WIDTH; }
    uint16_t get_height() const { return SCREEN_HEIGHT; }

    // Метод для доступа к PIO и SM (если нужен)
    PIO get_pio() const { return pio; }
    uint get_sm() const { return sm; }
};

#endif
