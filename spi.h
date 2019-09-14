#ifndef SPI_H__
#define SPI_H__

// SPI communication
//
// This module can be used for generic SPI communication.


#include <stdint.h>

#define SPIBUFLEN 4
extern uint8_t spi_buf_rx[SPIBUFLEN];
extern uint8_t spi_bytes_in_rx_buf;

uint8_t spi_busy();
void spi_busywait_until_done();
void spi_send(const uint8_t *buf, uint8_t len);

#endif
