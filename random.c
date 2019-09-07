#include "random.h"
#include "sha256.h"
#include "hardware.h"
#include <string.h>
#include <avr/eeprom.h>

struct psSha256_t rnd_container;
uint8_t rnd_hash[SHA256_HASHLEN];

void rnd_init() {
  psSha256Init(&rnd_container);

  rnd_feed_from_adc(32);

  // Feed the random pool from the eeprom. This will add device ID in to the
  // entropy meaning that different devices should get different initalization.
  eeprom_read_block(rnd_hash, 0, SHA256_HASHLEN);
  rnd_feed(rnd_hash, SHA256_HASHLEN);
}

void rnd_feed_from_adc(uint8_t count) {
  // Feed random pool by using the least significant bit read from ADC1.
  // (We add all the other bits as well, but they do not provide any good
  // entropy).
  uint16_t adc;
  uint8_t i;
  for (i = 0; i < count; i++) {
    adc = read_adc1();
    rnd_feed((uint8_t*)&adc, 2);
  }
}

void rnd_feed(uint8_t* data, uint8_t len) {
  psSha256Update(&rnd_container, data, len);
}

uint8_t* rnd_read() {
  // Do the finalization on a copy to preserve all states.
  struct psSha256_t rndTemp;
  memcpy(&rndTemp, &rnd_container, sizeof(rnd_container));
  psSha256Final(&rndTemp, rnd_hash);

  // Feed the random hash with something/anything to move it forward a bit.
  rnd_feed(rnd_hash, 2);
  return rnd_hash;
}

uint8_t rnd_integer(uint8_t max) {
  if (max == 0) {
    return 0; // The only random value that matches.
  }

  // Figure out the smallest number of bits we need to describe max. Do
  // this by counting the number of shifts needed to get zero. Create a
  // mask with this many ones.
  uint8_t mask;
  uint8_t i;
  for (i = 0; max >> i; i++) {
    ; // Loop until (max >> i) is zero and use resulting i for mask creation.
  }
  mask = (1 << i) - 1;

  while(1) {
    rnd_read(); // To update the rnd_hash
    for (i = 0; i < SHA256_HASHLEN; i++) {
      if ((rnd_hash[i] & mask) <= max) {
        // This random value (masked) was small enough for us. Return it.
        return (rnd_hash[i] & mask);
      }
      // Nope. Random value too big. Trow away.
    }
    // Ooops. All of the current hash was used. Make a new one (first in while).
  }
  return 0;
}
