#ifndef EEPROM_H__
#define EEPROM_H__

#include <stdint.h>

uint8_t eeReadByte(uint8_t* address);
void eeWriteByte(uint8_t* address, uint8_t byte);
#endif
