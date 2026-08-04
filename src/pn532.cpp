#include "include/pn532.h"

#include "hardware/i2c.h"
#include "hardware/gpio.h"

PN532::PN532(uint rx, uint tx, uint rsto, uint irq, uart_inst_t *uart = uart1)
:_rx(rx), _tx(tx), _rsto(rsto), _irq(irq), _uart(uart) {}

