#include "eeprom.h"
#include <avr/eeprom.h>
#include <avr/interrupt.h>

uint8_t eeReadByte(uint8_t* address) {
  uint8_t data;
  eeprom_busy_wait();
  cli();
  data = eeprom_read_byte(address);
  sei();
  return data;
}

void eeWriteByte(uint8_t* address, uint8_t byte) {
  eeprom_busy_wait();

  cli();
  eeprom_write_byte(address, byte);
  sei();
}
