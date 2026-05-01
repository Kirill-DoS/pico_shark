#ifndef  SI5351_H
#define SI5351_H

#include "pico/stdlib.h"

// default addr
#define SI5351_I2C_ADDR 0x60

// reg si5351
#define SI5351_REGISTER_0 0x00
#define SI5351_CLK0_CONTROL 0x16

// const
#define I2C_PORT i2c0
#define SDA_PIN 6
#define SCL_PIN 7
#define BAUDRATE 400 * 1000

class SI5351{

// write to reg
inline void si5351_write_reg(uint8_t reg, uint8_t data) {
    uint8_t buffer[2] = {reg, data};
    i2c_write_blocking(I2C_PORT, 0x60, buffer, 2, false);
}

// read reg Si5351
inline uint8_t si5351_read_reg(uint8_t reg) {
    uint8_t data;
    i2c_write_blocking(I2C_PORT, 0x60, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, 0x60, &data, 1, false);
    return data;
}

// reset Si5351
inline void si5351_reset() {
    si5351_write_reg(0x00, 0x00);  // Запись 0 в регистр 0
    sleep_ms(10);
    si5351_write_reg(0x00, 0x01);  // Запись 1 в регистр 0
}

void i2c_init();

bool si5351_detect();

};

#endif
