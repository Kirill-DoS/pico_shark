#ifndef TEMP_PRESS_H
#define TEMP_PRESS_H

#include <cstdint>

#define AHT20_ADDR 0x38
#define BMP280_ADDR 0x76
#define I2C_PORT i2c0



class Sensor{
	private:
		float temp;
		float press;
		float hum;
		int sda, scl;



	public:
		Sensor(int SDA, int SCL);
		//----I2C handle----
		void i2c_write_byte(uint8_t addr, uint8_t reg, uint8_t val);
		void i2c_read_bytes(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);
		//----AHT20 driver---
		void aht20_init();
		void aht20_read(float *humidity, float *temperature);
		//----BMP280 driver----
		void bmp280_init();
		void bmp280_read(float *temperature, float *pressure);
		void bmp280_read(float *pressure);

};

#endif
