#ifndef TEMP_H__
#define TEMP_H__

#include "sha256.h"

// The below union can be used whenever a tempoary variable is needed.
// Note however, that they must _not_ be used within interrupts.

#define MAX_TEMP_BUF 64

union Temp {
  uint8_t buf[MAX_TEMP_BUF];
  uint8_t hash[SHA256_HASHLEN];
  uint8_t ui8;
  int16_t i16;
};

extern union Temp temp;

#endif
