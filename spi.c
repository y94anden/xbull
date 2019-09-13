#include "spi.h"
#include "globals.h"
#include "hardware.h"

#include <avr/interrupt.h>

uint8_t spi_buf_tx[SPIBUFLEN];
uint8_t spi_buf_rx[SPIBUFLEN+1];
uint8_t spi_bytes_in_tx_buf;
uint8_t spi_bytes_in_rx_buf;

uint8_t spi_busy() {
  uint8_t bytes_left;
  cli();
  bytes_left = spi_bytes_in_tx_buf;
  sei();
  return bytes_left;
}

void spi_busywait_until_done() {
  while(spi_busy()) {
    ;
  }
}

void spi_send(const uint8_t *buf, uint8_t len) {
  if (len == 0) {
    // WTF?!?
    return;
  }
  spi_busywait_until_done();

  len = len > SPIBUFLEN+1 ? SPIBUFLEN+1 : len; // Truncate buffer

  for (spi_bytes_in_tx_buf=1;
       spi_bytes_in_tx_buf < len;
       spi_bytes_in_tx_buf++) {
    // Do not copy first byte. We will put that directly into sending buffer
    // Always place last byte first in the buffer. That way, we can send
    // buf[bytes_left-1], and when bytes_left == 0 we stop.
    spi_buf_tx[SPIBUFLEN-spi_bytes_in_tx_buf] = buf[spi_bytes_in_tx_buf];
  }
  spi_bytes_in_tx_buf--;
  spi_bytes_in_rx_buf = 0;

  spi_enable();
  SPDR = buf[0]; // Start sending.
}

void spi_finished() {
  // This function should determine what to do next. If nothing, disable spi.
  // The received data is in spi_buf_rx[0:spi_bytes_in_rx_buf] (python notaion)

  spi_disable();
}


ISR(SPI_STC_vect) {
  // A transmission is done

  // Read input
  spi_buf_rx[spi_bytes_in_rx_buf] = SPDR;
  spi_bytes_in_rx_buf++;

  // Check if we need to send more bytes.
  if (spi_bytes_in_tx_buf) {
    SPDR = spi_buf_tx[SPIBUFLEN - spi_bytes_in_tx_buf];
    spi_bytes_in_tx_buf--;
  } else {
    // No more bytes. Determine what to do next. Since this might take a while,
    // enable interrupts.
    sei();
    spi_finished();
  }
}
