#ifndef PN532_H
#define PN532_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define PN532_I2C_ADDRESS   (0x24)

#define PN532_COMMAND_GETFIRMWAREVERSION    (0x02)
#define PN532_COMMAND_SAMCONFIGURATION      (0x14)
#define PN532_COMMAND_INLISTPASSIVETARGET   (0x4A)
#define PN532_COMMAND_TGINITASTARGET        (0x8C)

#define PN532_MIFARE_ISO14443A              (0x00)

class PN532 {
public:
	PN532(i2c_inst_t *i2c_instance, uint sda_pin, uint scl_pin);

	bool begin();
	bool getFirmwareVersion(uint32_t &version);
	bool samConfig();

	bool readPassiveTargetID(uint8_t *uid, uint8_t &uidLength);

	// Добавлено для совместимости с твоим проектом.
	// Если эмуляция не нужна - можно удалить.
	bool emulateMifareClassic(
		const uint8_t *uid,
		uint8_t uid_len,
		uint16_t timeout_ms = 10000
	);
	static bool scan_in_progress;
	static bool data_ready;
private:
	i2c_inst_t *_i2c;
	uint _sda;
	uint _scl;

	bool sendCommand(const uint8_t *cmd, uint8_t cmdLen);
	bool readResponse(uint8_t *reply, uint8_t replyLen, uint16_t max_timeout_ms = 1000);
	bool checkAck();
};

#endif // PN532_H
