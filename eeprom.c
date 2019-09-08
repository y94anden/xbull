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

void eeReadBlock(uint8_t* address, uint8_t *buffer, uint8_t length) {
  eeprom_busy_wait();
  cli();
  eeprom_read_block(buffer, address, length);
  sei();
}

void eeWriteBlock(uint8_t* address, const uint8_t* buffer, uint8_t length) {
  eeprom_busy_wait();
  cli();
  eeprom_update_block(buffer, address, length);
  sei();
}
