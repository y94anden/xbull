#ifndef SPI_H__
#define SPI_H__

#include <stdint.h>

#define SPIBUFLEN 3 // One less than we want to be able to send
extern uint8_t spi_buf_rx[SPIBUFLEN+1];
extern uint8_t spi_bytes_in_rx_buf;

uint8_t spi_busy();
void spi_busywait_until_done();
void spi_send(const uint8_t *buf, uint8_t len);

#endif
