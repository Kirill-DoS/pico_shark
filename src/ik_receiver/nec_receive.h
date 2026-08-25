#ifndef NEC_RECEIVE_H
#define NEC_RECEIVE_H

#include "pico/stdlib.h"
#include "hardware/pio.h"

// public API
class nec_receive{
	public:
		int nec_rx_init(PIO pio, uint pin);
		bool nec_decode_frame(uint32_t sm, uint8_t *p_address, uint8_t *p_data);

};

#endif
