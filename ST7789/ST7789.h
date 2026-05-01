#pragma once
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "PicoGFX.h"
#include "def.h"
#include <cstdint>  // для uint16_t

class ST7789 : public PicoGFX {
public:
    ST7789(uint cs, uint dc, uint rst, spi_inst_t* spi = spi0);

    void begin();
    void setRotation(uint8_t r);

    // Методы от PicoGFX
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;

    // Методы для LVGL (работают с RGB565 = uint16_t)
    void pushPixels(const uint16_t* pixels, uint32_t count);
    void setWindow(int16_t x, int16_t y, int16_t w, int16_t h);
    void pushColors(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* colors);

    // Flush-метод для LVGL (принимает uint16_t*, не lv_color_t!)
    void flushBuffer(int16_t x1, int16_t y1, int16_t x2, int16_t y2, const uint16_t* color_p);

private:
    void writeCmd(uint8_t cmd);
    void writeData(const uint8_t* buf, size_t len);
    void setAddrWindow(int16_t x, int16_t y, int16_t w, int16_t h);

    uint _cs, _dc, _rst;
    spi_inst_t* _spi;
};
