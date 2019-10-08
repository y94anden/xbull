#include "dht11.h"
#include "hardware.h"
#include <string.h>  // For memset
#include <util/delay.h>

uint8_t dht_wait(uint8_t requested_bit) {
  uint8_t wait;
  for (wait=100; wait > 0; wait--) {
    if (requested_bit) {
      if (dht_pin()) {
        // We want a one and got a one.
        return 0;
      }
    } else {
      if (!dht_pin()) {
        // We want a zero and got a zero.
        return 0;
      }
    }
    _delay_us(1);
  }

  // Timeout.
  return 1;
}

uint8_t dht_verify_checksum(uint8_t buf[5]) {
  /*
    Data consists of decimal and integral parts. A complete data transmission
    is 40bit, and the sensor sends higher data bit first.
    Data format: 8bit integral RH data + 8bit decimal RH data + 8bit integral
    T data + 8bit decimal T data + 8bit check sum.

    If the data transmission is right, the check-sum should be the last 8bit of
    the sum of the first four bytes.
  */
  uint8_t sum = buf[0] + buf[1] + buf[2] + buf[3];
  return sum == buf[4];
}

uint8_t dht_read(uint8_t buf[5]) {
  uint8_t i;

  memset(buf, 0x0, 5);

  dht_pin_low();
  _delay_ms(18);
  dht_pin_input();

  // The bus is now pulled high by pullups, and the DHT11 will reply
  // with a zero within 20-40 µs. This zero will last for 80 µs.
  _delay_us(40);
  if (dht_pin() != 0) {
    // dht11 did not respond.
    return 1;
  }

  // Wait for bus to go high again. When high, start waiting for transmission.
  if(dht_wait(1)) {
    return 2; // Oops. Timeout waiting for bus release
  }

  for (i = 0; i < 40; i++) {
    // Wait for start of bit transmission
    if (dht_wait(0)) {
      return 3; // Timeout. No startbit.
    }

    // Wait for bus to go high to start measure time for 1 or 0.
    if (dht_wait(1)) {
      return 4;
    }

    _delay_us(30); // Wait beyond the length of a zero and sample.
    buf[i/8] <<= 1;
    buf[i/8] |= dht_pin();
  }

  if (!dht_verify_checksum(buf)) {
    return 5;
  }

  return 0;
}

