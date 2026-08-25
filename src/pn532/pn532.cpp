#include "pn532.h"
#include <cstdio>
#include <cstring>

bool PN532::scan_in_progress = false;
bool PN532::data_ready = false;

PN532::PN532(i2c_inst_t *i2c_instance, uint sda_pin, uint scl_pin) {
	_i2c = i2c_instance;
	_sda = sda_pin;
	_scl = scl_pin;
}

bool PN532::begin() {
	i2c_init(_i2c, 100 * 1000);

	gpio_set_function(_sda, GPIO_FUNC_I2C);
	gpio_set_function(_scl, GPIO_FUNC_I2C);

	gpio_pull_up(_sda);
	gpio_pull_up(_scl);

	samConfig();

	return true;
}

bool PN532::sendCommand(const uint8_t *cmd, uint8_t cmdLen) {
	uint8_t packet[64];

	if ((size_t)cmdLen + 8 > sizeof(packet)) {
		return false;
	}

	uint8_t checksum = 0;

	packet[0] = 0x00;
	packet[1] = 0x00;
	packet[2] = 0xFF;
	packet[3] = cmdLen + 1;
	packet[4] = (uint8_t)(~(cmdLen + 1)) + 1;
	packet[5] = 0xD4;

	checksum += 0xD4;

	for (uint8_t i = 0; i < cmdLen; i++) {
		packet[6 + i] = cmd[i];
		checksum += cmd[i];
	}

	packet[6 + cmdLen] = (uint8_t)(~checksum) + 1;
	packet[7 + cmdLen] = 0x00;

	int result = i2c_write_blocking(
		_i2c,
		PN532_I2C_ADDRESS,
		packet,
		cmdLen + 8,
		false
	);

	if (result == PICO_ERROR_GENERIC) {
		printf("[PN532 ERR] i2c_write_blocking failed\n");
		return false;
	}

	return checkAck();
}

bool PN532::checkAck() {
	uint8_t ackBuf[7];
	uint16_t timeout = 1000;

	while (timeout > 0) {
		uint8_t status = 0;

		i2c_read_blocking(
			_i2c,
			PN532_I2C_ADDRESS,
			&status,
			1,
			false
		);

		if (status == 0x01) {
			break;
		}

		sleep_ms(1);
		timeout--;
	}

	if (timeout == 0) {
		printf("[PN532 ERR] ACK wait timeout\n");
		return false;
	}

	int result = i2c_read_blocking(
		_i2c,
		PN532_I2C_ADDRESS,
		ackBuf,
		7,
		false
	);

	if (result == PICO_ERROR_GENERIC) {
		return false;
	}

	if (ackBuf[1] == 0x00 &&
		ackBuf[2] == 0x00 &&
		ackBuf[3] == 0xFF &&
		ackBuf[4] == 0x00 &&
		ackBuf[5] == 0xFF &&
		ackBuf[6] == 0x00) {
		return true;
		}

		printf("[PN532 ERR] bad ACK\n");
	return false;
}

bool PN532::readResponse(uint8_t *reply, uint8_t replyLen, uint16_t max_timeout_ms) {
	uint16_t timeout = max_timeout_ms;

	while (timeout > 0) {
		uint8_t status = 0;

		int ret = i2c_read_blocking(
			_i2c,
			PN532_I2C_ADDRESS,
			&status,
			1,
			false
		);

		if (ret >= 0 && status == 0x01) {
			break;
		}

		sleep_ms(1);
		timeout--;
	}

	if (timeout == 0) {
		return false;
	}

	uint8_t buf[64];

	if ((size_t)replyLen + 8 > sizeof(buf)) {
		return false;
	}

	memset(buf, 0, sizeof(buf));

	int result = i2c_read_blocking(
		_i2c,
		PN532_I2C_ADDRESS,
		buf,
		replyLen + 8,
		false
	);

	if (result == PICO_ERROR_GENERIC) {
		return false;
	}

	for (uint8_t i = 0; i < replyLen; i++) {
		reply[i] = buf[i + 6];
	}

	return true;
}

bool PN532::getFirmwareVersion(uint32_t &version) {
	uint8_t cmd[] = { PN532_COMMAND_GETFIRMWAREVERSION };

	if (!sendCommand(cmd, 1)) {
		return false;
	}

	uint8_t reply[6];

	if (!readResponse(reply, 6)) {
		return false;
	}

	if (reply[0] != 0xD5 || reply[1] != 0x03) {
		return false;
	}

	version = reply[2];
	version <<= 8;
	version |= reply[3];
	version <<= 8;
	version |= reply[4];
	version <<= 8;
	version |= reply[5];

	return true;
}

bool PN532::samConfig() {
	uint8_t cmd[] = {
		PN532_COMMAND_SAMCONFIGURATION,
		0x01,
		0x14,
		0x01
	};

	if (!sendCommand(cmd, 4)) {
		return false;
	}

	uint8_t reply[2];

	return readResponse(reply, 2);
}

bool PN532::readPassiveTargetID(uint8_t *uid, uint8_t &uidLength) {
	uint8_t cmd[] = {
		PN532_COMMAND_INLISTPASSIVETARGET,
		0x01,
		PN532_MIFARE_ISO14443A
	};

	if (!sendCommand(cmd, 3)) {
		printf("[NFC] sendCommand failed\n");
		return false;
	}

	uint8_t reply[20];
	memset(reply, 0, sizeof(reply));

	// было 30 мс — чип не успевает. Ставим 300 мс.
	if (!readResponse(reply, 20, 30)) {
		printf("[NFC] readResponse TIMEOUT\n");
		return false;
	}

	// что реально прислал чип
	printf("[NFC] reply: ");
	for (int i = 0; i < 12; i++) printf("%02X ", reply[i]);
	printf("\n");

	if (reply[2] < 1) {
		printf("[NFC] 0 targets\n");
		return false;
	}

	uidLength = reply[7];
	if (uidLength > 7) uidLength = 7;

	for (uint8_t i = 0; i < uidLength; i++) {
		uid[i] = reply[8 + i];
	}

	return true;
}

bool PN532::emulateMifareClassic(
	const uint8_t *uid,
	uint8_t uid_len,
	uint16_t timeout_ms
) {
	if (uid == nullptr || uid_len != 4) {
		return false;
	}

	uint8_t cmd[38];
	memset(cmd, 0, sizeof(cmd));

	cmd[0] = PN532_COMMAND_TGINITASTARGET;

	// Режим: эмулировать карту.
	// Если с 0x01 будут проблемы, можно попробовать 0x00.
	cmd[1] = 0x01;

	// MIFARE parameters
	cmd[2] = 0x04;
	cmd[3] = 0x00;

	cmd[4] = uid[0];
	cmd[5] = uid[1];
	cmd[6] = uid[2];

	cmd[7] = 0x08;

	// FelicaParams и NFCID3t остаются нулями.
	// Длина General Bytes и Historical Bytes тоже 0.

	printf(
		"[PN532 EMU] start emulation UID: %02X %02X %02X %02X\n",
		uid[0],
		uid[1],
		uid[2],
		uid[3]
	);

	if (!sendCommand(cmd, sizeof(cmd))) {
		printf("[PN532 EMU] TgInitAsTarget send failed\n");
		return false;
	}

	uint8_t reply[16];

	return readResponse(reply, sizeof(reply), timeout_ms);
}
