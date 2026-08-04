#ifndef PN532_H
#define PN532_H

#include <cstddef>
#include <cstdint>
#include <sys/types.h>

#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

class PN532{
private:
	uart_inst_t* _uart;
	uint _rx, _tx, _rsto, _irq;
	void send_cmd(const uint8_t* cmd, size_t len);
	bool read_resp(uint8_t* buffer, size_t expected_len, uint32_t timeout_ms = 100);

public:
	PN532(uint rx, uint tx, uint rsto, uint irq, uart_inst_t *uart = uart1);
	bool init();

	void start_passive_target_detectin();
	bool read_detection_card_uid(uint8_t* uid, uint8_t* uid_len);
}

#endif
