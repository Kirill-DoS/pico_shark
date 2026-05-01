#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef PROGMEM
#define PROGMEM
#endif

typedef struct {
    uint16_t bitmapOffset;
    uint8_t  width, height;
    uint8_t  xAdvance;
    int8_t   xOffset, yOffset;
} GFXglyph;

typedef struct {
    uint8_t   *bitmap;
    GFXglyph  *glyph;
    uint16_t   first, last;
    uint8_t    yAdvance;
} GFXfont;

class PicoGFX {
public:
    PicoGFX(int16_t w, int16_t h) : _width(w), _height(h) {}
    virtual ~PicoGFX() = default;

    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;
    virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;

    void fillScreen(uint16_t color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

    void setFont(const GFXfont *f = nullptr);
    void setCursor(int16_t x, int16_t y) { cursor_x = x; cursor_y = y; }
    void setTextSize(uint8_t s) { textsize_x = textsize_y = (s > 0) ? s : 1; }
    void setTextSize(uint8_t sx, uint8_t sy) { textsize_x = (sx>0)?sx:1; textsize_y=(sy>0)?sy:1; }
    void setTextWrap(bool w) { wrap = w; }
    void setTextColor(uint16_t c) { textcolor = c; textbgcolor = 0; }
    void setTextColor(uint16_t c, uint16_t bg) { textcolor = c; textbgcolor = bg; }

    size_t print(const char* str);
    size_t println(const char* str = "");
    size_t print(int n, int base = 10);
    size_t println(int n, int base = 10);
    size_t print(char c) { write((uint8_t)c); return 1; }

    int16_t width() const { return _width; }
    int16_t height() const { return _height; }

protected:
    int16_t _width, _height;
    int16_t cursor_x = 0, cursor_y = 0;
    uint8_t textsize_x = 1, textsize_y = 1;
    uint16_t textcolor = 0xFFFF, textbgcolor = 0;
    const GFXfont *_gfxFont = nullptr;
    bool wrap = true;
    bool _cp437 = false;

    void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y);
    virtual size_t write(uint8_t c);
};
