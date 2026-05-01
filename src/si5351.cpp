#include "include/si5351.h"
#include "hardware/i2c.h"

void i2c_init(){
    i2c_init(I2C_PORT, BAUDRATE);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
}

bool si5351_detect() {
    uint8_t data;
    // Пробуем прочитать регистр 0
    int result = i2c_read_blocking(i2c0, 0x60, &data, 1, false);
    return result >= 0;  // Если >= 0, устройство ответило
}
