// commands ST7789
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

// collors ST7789 RGB565
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

// dislay size
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// pinout
#define MOSI 19
#define SCK 18
#define MISO 16
