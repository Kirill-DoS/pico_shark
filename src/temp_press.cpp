#include "include/temp_press.h"

#include "hardware/i2c.h"
#include "hardware/gpio.h"

//calib coef
uint16_t dig_T1;
int16_t dig_T2, dig_T3;
uint16_t dig_P1;
int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
int32_t t_fine;

Sensor::Sensor(int SDA, int SCL){
	sda = SDA;
	scl = SCL;
	i2c_init(I2C_PORT, 100 * 1000);
	gpio_set_function(sda, GPIO_FUNC_I2C);
	gpio_set_function(scl, GPIO_FUNC_I2C);
	gpio_pull_up(sda);
	gpio_pull_up(scl);
}

void Sensor::i2c_write_byte(uint8_t addr, uint8_t reg, uint8_t val) {
	uint8_t buf[2] = {reg, val};
	i2c_write_blocking(I2C_PORT, addr, buf, 2, false);
}

void Sensor::i2c_read_bytes(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len) {
	i2c_write_blocking(I2C_PORT, addr, &reg, 1, true);
	i2c_read_blocking(I2C_PORT, addr, buf, len, false);
}

void Sensor::aht20_init() {
	sleep_ms(40); // Время на запуск датчика
	uint8_t cmd[] = {0x71}; // Команда чтения статуса
	uint8_t status;
	i2c_write_blocking(I2C_PORT, AHT20_ADDR, cmd, 1, true);
	i2c_read_blocking(I2C_PORT, AHT20_ADDR, &status, 1, false);

	// Если датчик не калиброван, отправляем команду инициализации
	if ((status & 0x08) == 0) {
		uint8_t init_cmd[] = {0xBE, 0x08, 0x00};
		i2c_write_blocking(I2C_PORT, AHT20_ADDR, init_cmd, 3, false);
		sleep_ms(10);
	}
}

void Sensor::aht20_read(float *humidity, float *temperature) {
	uint8_t measure_cmd[] = {0xAC, 0x33, 0x00};
	i2c_write_blocking(I2C_PORT, AHT20_ADDR, measure_cmd, 3, false);
	sleep_ms(80); // Ожидание окончания измерения

	uint8_t data[7];
	i2c_read_blocking(I2C_PORT, AHT20_ADDR, data, 7, false);

	// Проверка бита готовности (data[0] & 0x80 == 0)
	if ((data[0] & 0x80) == 0) {
		uint32_t raw_humidity = (((uint32_t)data[1]) << 12) | (((uint32_t)data[2]) << 4) | ((data[3] & 0xF0) >> 4);
		uint32_t raw_temperature = (((uint32_t)(data[3] & 0x0F)) << 16) | (((uint32_t)data[4]) << 8) | data[5];

		*humidity = ((float)raw_humidity / 1048576.0f) * 100.0f;
		*temperature = ((float)raw_temperature / 1048576.0f) * 200.0f - 50.0f;
	}
}

void Sensor::bmp280_init() {
	uint8_t calib[24];
	i2c_read_bytes(BMP280_ADDR, 0x88, calib, 24);

	// Считывание калибровочных значений из памяти датчика
	dig_T1 = (calib[1] << 8) | calib[0];
	dig_T2 = (calib[3] << 8) | calib[2];
	dig_T3 = (calib[5] << 8) | calib[4];
	dig_P1 = (calib[7] << 8) | calib[6];
	dig_P2 = (calib[9] << 8) | calib[8];
	dig_P3 = (calib[11] << 8) | calib[10];
	dig_P4 = (calib[13] << 8) | calib[12];
	dig_P5 = (calib[15] << 8) | calib[14];
	dig_P6 = (calib[17] << 8) | calib[16];
	dig_P7 = (calib[19] << 8) | calib[18];
	dig_P8 = (calib[21] << 8) | calib[20];
	dig_P9 = (calib[23] << 8) | calib[22];

	// Настройка режима: Normal mode, Усреднение Temp x1, Press x1
	i2c_write_byte(BMP280_ADDR, 0xF4, 0x27);
	// Настройка фильтра (выключен) и времени ожидания (0.5ms)
	i2c_write_byte(BMP280_ADDR, 0xF5, 0x00);
}

void Sensor::bmp280_read(float *temperature, float *pressure) {
	uint8_t data[6];
	i2c_read_bytes(BMP280_ADDR, 0xF7, data, 6);

	int32_t adc_P = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | (data[2] >> 4);
	int32_t adc_T = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | (data[5] >> 4);

	// Компенсация температуры (из даташита Bosch)
	int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
	int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
	t_fine = var1 + var2;
	*temperature = (float)((t_fine * 5 + 128) >> 8) / 100.0f;

	// Компенсация давления (из даташита Bosch)
	int64_t v1 = ((int64_t)t_fine) - 128000;
	int64_t v2 = v1 * v1 * (int64_t)dig_P6;
	v2 = v2 + ((v1 * (int64_t)dig_P5) << 17);
	v2 = v2 + (((int64_t)dig_P4) << 35);
	v1 = ((v1 * v1 * (int64_t)dig_P3) >> 8) + ((v1 * (int64_t)dig_P2) << 12);
	v1 = (((((int64_t)1) << 47) + v1)) * ((int64_t)dig_P1) >> 33;

	if (v1 == 0) {
		*pressure = 0.0f; // избегаем деления на ноль
		return;
	}

	int64_t p = 1048576 - adc_P;
	p = (((p << 31) - v2) * 3125) / v1;
	v1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
	v2 = (((int64_t)dig_P8) * p) >> 19;
	p = ((p + v1 + v2) >> 8) + (((int64_t)dig_P7) << 4);

	*pressure = (float)p / 256.0f; // Давление в Паскалях
}

void Sensor::bmp280_read(float *pressure) {
	uint8_t data[6];
	i2c_read_bytes(BMP280_ADDR, 0xF7, data, 6);

	int32_t adc_P = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | (data[2] >> 4);
	int32_t adc_T = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | (data[5] >> 4);

	// Компенсация давления (из даташита Bosch)
	int64_t v1 = ((int64_t)t_fine) - 128000;
	int64_t v2 = v1 * v1 * (int64_t)dig_P6;
	v2 = v2 + ((v1 * (int64_t)dig_P5) << 17);
	v2 = v2 + (((int64_t)dig_P4) << 35);
	v1 = ((v1 * v1 * (int64_t)dig_P3) >> 8) + ((v1 * (int64_t)dig_P2) << 12);
	v1 = (((((int64_t)1) << 47) + v1)) * ((int64_t)dig_P1) >> 33;

	if (v1 == 0) {
		*pressure = 0.0f; // избегаем деления на ноль
		return;
	}

	int64_t p = 1048576 - adc_P;
	p = (((p << 31) - v2) * 3125) / v1;
	v1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
	v2 = (((int64_t)dig_P8) * p) >> 19;
	p = ((p + v1 + v2) >> 8) + (((int64_t)dig_P7) << 4);

	*pressure = (float)p / 256.0f; // Давление в Паскалях
}
