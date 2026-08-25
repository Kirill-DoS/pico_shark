#ifndef PINS_H
#define PINS_H

// display
#define SCL_DISP 	11
#define SDA_DISP 	10
#define CS_DISP 	12
#define DC_DISP 	14
#define RST_DISP 	15
#define SPI_PORT    spi1
// AHT20 + BMP280
#define SDA_I2C  	0
#define SCL_I2C  	1
#define I2C_PORT	i2c0
// PN532
#define SDA_PN   	0
#define SCL_PN   	1
#define IRQ_PN   	2
#define RSTO_PN  	3

// enkoder
#define CLK_ENK 	20
#define DT_ENK  	21
#define SW_ENK  	22

// microSD
#define MISO_SD 	16
#define MOSI_SD 	19
#define SCK_SD  	18
#define CS_SD   	17
// free pins:4,5,6,7,8,9

#endif
