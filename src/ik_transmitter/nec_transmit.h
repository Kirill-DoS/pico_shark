#ifndef NEC_TRANSMIT_H
#define NEC_TRANSMIT_H

#include "pico/stdlib.h"
#include "hardware/pio.h"

// public API
class nec_transmit{
	public:
		int nec_tx_init(PIO pio, uint pin);
		uint32_t nec_encode_frame(uint8_t address, uint8_t data);

};

#endif
