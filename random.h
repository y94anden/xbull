#ifndef RANDOM_H__
#define RANDOM_H__

// Keep some random bytes handy if needed. The bytes are
// Generated by reading the least significant analog bit
// of some pin with a pullup (could for instance be connected
// to a button).

#include <stdint.h>

void rnd_init();
void rnd_feed_from_adc(uint8_t count);
void rnd_feed(uint8_t* data, uint8_t len);
uint8_t* rnd_read();
uint8_t rnd_integer(uint8_t max);

#endif
