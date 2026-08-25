#include "ST7789.h"
#include <cinttypes>
#include <cstdint>
#include "../pins.h"
ST7789::ST7789(uint cs, uint dc, uint rst, spi_inst_t* spi)
: PicoGFX(SCREEN_WIDTH, SCREEN_HEIGHT), _cs(cs), _dc(dc), _rst(rst), _spi(spi) {}

void ST7789::begin() {
    gpio_init(_cs); gpio_set_dir(_cs, GPIO_OUT); gpio_put(_cs, 1);
    gpio_init(_dc); gpio_set_dir(_dc, GPIO_OUT);
    gpio_init(_rst); gpio_set_dir(_rst, GPIO_OUT);

    spi_init(_spi, 62000000); // ↓ СНИЗИЛИ до 10 МГц для стабильности!
    gpio_set_function(SDA_DISP, GPIO_FUNC_SPI); // MOSI
    gpio_set_function(SCL_DISP, GPIO_FUNC_SPI); // SCK
    gpio_set_function(MISO, GPIO_FUNC_SPI); // MISO (для стабильности шины)

    // Сброс
    gpio_put(_rst, 0); sleep_ms(50); gpio_put(_rst, 1); sleep_ms(200);

    // Последовательность инициализации
    const uint8_t init_cmds[] = {
        1, 20,  ST7789_SWRESET,      // 1 байт (команда), 20*5мс задержка, команда SWRESET
        1, 10,  ST7789_SLPOUT,       // 1 байт (команда), 10*5мс задержка, команда SLPOUT
        2, 0,   ST7789_COLMOD, 0x55, // 2 байта (команда + данные), 0 задержка
        2, 0,   ST7789_MADCTL, 0x60, // Настройка ориентации
        5, 0,   ST7789_CASET, 0x00, 0x00, SCREEN_WIDTH >> 8, SCREEN_WIDTH & 0xFF,
        5, 0,   ST7789_RASET, 0x00, 0x00, SCREEN_HEIGHT >> 8, SCREEN_HEIGHT & 0xFF,
        1, 0,   ST7789_INVON,
        1, 0,   0x13,
        1, 0,   ST7789_DISPON,
        0                           // Конец массива
    };

    const uint8_t *cmd = init_cmds;
    while (*cmd) {
        uint8_t len = *cmd++;     // Количество байт (команда + данные)
        uint8_t delay = *cmd++;   // Множитель задержки
        uint8_t command = *cmd++; // Сама команда

        writeCmd(command);        // Правильно переключает DC в 0
        if (len > 1) {
            writeData(cmd, len - 1); // Правильно переключает DC в 1
            cmd += (len - 1);
        }

        if (delay > 0) sleep_ms(delay * 5);
    }

    fillScreen(0x0000);
}

void ST7789::writeCmd(uint8_t cmd) {
    gpio_put(_cs, 0); gpio_put(_dc, 0);
    spi_write_blocking(_spi, &cmd, 1);
    gpio_put(_cs, 1);
}

void ST7789::writeData(const uint8_t* buf, size_t len) {
    gpio_put(_cs, 0); gpio_put(_dc, 1);
    spi_write_blocking(_spi, buf, len);
    gpio_put(_cs, 1);
}

void ST7789::setAddrWindow(int16_t x, int16_t y, int16_t w, int16_t h) {
    uint16_t X_OFFSET = 0;
    uint16_t Y_OFFSET = 0;

    x += X_OFFSET;
    y += Y_OFFSET;

    uint16_t x2 = x + w - 1;
    uint16_t y2 = y + h - 1;
    // Явные касты для устранения предупреждений narrowing
    uint8_t d[4] = {
        static_cast<uint8_t>((x >> 8) & 0xFF),
        static_cast<uint8_t>(x & 0xFF),
        static_cast<uint8_t>((x2 >> 8) & 0xFF),
        static_cast<uint8_t>(x2 & 0xFF)
    };
    writeCmd(ST7789_CASET); writeData(d, 4);
    d[0] = static_cast<uint8_t>((y >> 8) & 0xFF);
    d[1] = static_cast<uint8_t>(y & 0xFF);
    d[2] = static_cast<uint8_t>((y2 >> 8) & 0xFF);
    d[3] = static_cast<uint8_t>(y2 & 0xFF);
    writeCmd(ST7789_RASET); writeData(d, 4);
    writeCmd(ST7789_RAMWR);
}

void ST7789::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || x >= _width || y < 0 || y >= _height) return;
    setAddrWindow(x, y, 1, 1);
    uint8_t c[2] = {
        static_cast<uint8_t>(color >> 8),
        static_cast<uint8_t>(color & 0xFF)
    };
    gpio_put(_cs, 0); gpio_put(_dc, 1);
    spi_write_blocking(_spi, c, 2);
    gpio_put(_cs, 1);
}

void ST7789::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (x >= _width || y >= _height) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > _width) w = _width - x;
    if (y + h > _height) h = _height - y;  // ← перенесли на новую строку для читаемости

    setAddrWindow(x, y, w, h);
    uint8_t c[2] = {
        static_cast<uint8_t>(color >> 8),
        static_cast<uint8_t>(color & 0xFF)
    };
    gpio_put(_cs, 0); gpio_put(_dc, 1);
    for (uint32_t i = 0; i < static_cast<uint32_t>(w) * h; i++) {
        spi_write_blocking(_spi, c, 2);
    }
    gpio_put(_cs, 1);
}

void ST7789::setRotation(uint8_t r) {
    writeCmd(0x36);
    uint8_t m[] = {0x00, 0x60, 0xC0, 0xA0};
    writeData(&m[r & 3], 1);
}

void ST7789::pushPixels(const uint16_t* pixels, uint32_t count) {
    gpio_put(_cs, 0);
    gpio_put(_dc, 1);
    spi_write16_blocking(_spi, pixels, count);
    gpio_put(_cs, 1);
}

void ST7789::setWindow(int16_t x, int16_t y, int16_t w, int16_t h) {
    setAddrWindow(x, y, w, h);
}

void ST7789::pushColors(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* colors) {
    setAddrWindow(x, y, w, h);
    gpio_put(_cs, 0);
    gpio_put(_dc, 1);

    uint32_t len = (uint32_t)w*h;
    for(uint32_t i = 0; i < len; i++){
        uint16_t c = colors[i];
        // c = (c << 8) | (c >> 8);
        uint8_t d[2] = {(uint8_t)(c >> 8), (uint8_t)(c & 0xFF)};
        spi_write_blocking(_spi, d, 2);
    }
    gpio_put(_cs, 1);
}

void ST7789::flushBuffer(int16_t x1, int16_t y1, int16_t x2, int16_t y2, const uint16_t* color_p) {
    setAddrWindow(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
    pushPixels(color_p, static_cast<uint32_t>(x2 - x1 + 1) * (y2 - y1 + 1));
}
