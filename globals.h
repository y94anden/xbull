#ifndef TEMP_H__
#define TEMP_H__

#include "sha256.h"

// The below union can be used whenever a tempoary variable is needed.
// Note however, that they must _not_ be used within interrupts.

#define MAX_TEMP_BUF 64

union Temp {
  uint8_t  buf[MAX_TEMP_BUF];
  uint8_t  hash[SHA256_HASHLEN];
  uint8_t  ui8;
  int16_t  i16;
  uint16_t ui16;
};

extern union Temp temp;


#define SERIALBUFSIZE 140  // Programming flash has a payload of 129 bytes.
#if SPM_PAGESIZE != 128
  #error This code is written for a SPM_PAGESIZE of 128 bytes.
#endif
#if SERIALBUFSIZE < SPM_PAGESIZE + 5
  #error The serial buffer cannot hold a full page as data.
#endif

extern uint8_t serialbuffer[SERIALBUFSIZE];

#endif
