#include <stdio.h>
#include "pico/stdlib.h"
#include "../../src/pn532/pn532.h"

#define I2C_PORT  i2c0
#define SDA_PIN   0
#define SCL_PIN   1

int main() {
	stdio_init_all();
	sleep_ms(2000);
	printf("Инициализация PN532 по I2C...\\n");

	PN532 nfc(I2C_PORT, SDA_PIN, SCL_PIN);
	if(nfc.begin()){
		printf("Инициализация успешна\n");
	}else{
		printf("Инициализация провалилась\n");
	}

	uint32_t version = 0;
	if (!nfc.getFirmwareVersion(version)) {
		printf("PN532 не найден! Проверьте подключение.\\n");
		while (1) sleep_ms(1000);
	}

	printf("Найден чип PN532. Версия прошивки: 0x%08X\\n", version);
	nfc.samConfig(); // Включение SAM (Secure Access Module)

	printf("Ожидание NFC/RFID метки...\\n");

	while (1) {
		uint8_t uid[7]; // Массив под UID (может быть до 7 байт для Mifare Ultralight/DESFire)
		uint8_t uidLength = 0;

		// Вызываем чтение без лишнего шума в главном цикле
		if (nfc.readPassiveTargetID(uid, uidLength)) {
			printf("\n=== УСПЕХ: КАРТА ПРИНЯТА ===");
			printf("Длина UID: %d байт\n", uidLength);
			printf("UID: ");
			for (uint8_t i = 0; i < uidLength; i++) {
				printf("0x%02X ", uid[i]);
			}
			printf("\n============================\n\n");
			sleep_ms(1000);
		}
		sleep_ms(200); // Небольшая пауза между сканированиями шины
	}
}
