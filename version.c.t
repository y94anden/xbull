#include "version.h"
#include <avr/pgmspace.h>

const char strVERSION[] PROGMEM = "PLACEHOLDER";

uint8_t version_length() {
  uint8_t pos = 0;

  // Calculate length from progmem
  while(pgm_read_byte(&(strVERSION[pos])) != 0) {
    pos++;
  }
  return pos;
}

uint8_t version_char(uint8_t pos) {
  return pgm_read_byte(&(strVERSION[pos]));
}
