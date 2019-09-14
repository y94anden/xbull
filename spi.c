#include "spi.h"
#include "globals.h"
#include "hardware.h"

#include <avr/interrupt.h>

uint8_t spi_buf_tx[SPIBUFLEN-1]; // First byte sent needs no buffer
uint8_t spi_buf_rx[SPIBUFLEN];
uint8_t spi_bytes_in_tx_buf;
uint8_t spi_bytes_in_rx_buf;

uint8_t spi_busy() {
  uint8_t busy;
  cli();
  busy = spi_bytes_in_tx_buf & 0x80;
  sei();
  return busy;
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

  len = len > SPIBUFLEN ? SPIBUFLEN : len; // Truncate buffer

  for (spi_bytes_in_tx_buf=1;
       spi_bytes_in_tx_buf < len;
       spi_bytes_in_tx_buf++) {
    // Do not copy first byte. We will put that directly into sending buffer
    // Always place last byte first in the buffer. That way, we can send
    // buf[bytes_left-1], and when bytes_left == 0 we stop.
    spi_buf_tx[(SPIBUFLEN-1)-len + spi_bytes_in_tx_buf] = buf[spi_bytes_in_tx_buf];
  }
  spi_bytes_in_tx_buf--; // It was == len when aborting for loop
  spi_bytes_in_tx_buf |= 0x80; // Use last bit as indicator that we are still sending.
  spi_bytes_in_rx_buf = 0;

  spi_enable();
  SPDR = buf[0]; // Start sending.
}

void spi_finished() {
  // This function should determine what to do next.
  // The received data is in spi_buf_rx[0:spi_bytes_in_rx_buf] (python notaion)
}


ISR(SPI_STC_vect) {
  // A transmission is done

  // Read input
  spi_buf_rx[spi_bytes_in_rx_buf] = SPDR;
  spi_bytes_in_rx_buf++;

  // Check if we need to send more bytes.
  if (spi_bytes_in_tx_buf & 0x7F) {
    SPDR = spi_buf_tx[(SPIBUFLEN-1) - (spi_bytes_in_tx_buf & 0x7F)];
    spi_bytes_in_tx_buf--;
  } else {
    // No more bytes. Determine what to do next. Since this might take a while,
    // enable interrupts.
    sei();
    spi_bytes_in_tx_buf = 0; // Reset the sending bit to indicate we are done.
    spi_finished();
  }
}
