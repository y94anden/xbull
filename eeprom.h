#ifndef EEPROM_H__
#define EEPROM_H__

#include <stdint.h>

// Usage of addresses in eeprom:
//
// 0x00 Address of device, 1 byte (bull.c)
// 0x01 -
// ...  | Name of device
// 0x0F -
// 0x10 Bitmask of current running state. 1 == do not listen to incoming uart.
// 0x20 -
// ...  | Mapped to parameters 0x10-0x1F, 1 byte per parameter (bull.c)
// 0x2f -

uint8_t eeReadByte(uint8_t* address);
void eeWriteByte(uint8_t* address, uint8_t byte);
void eeReadBlock(uint8_t* address, uint8_t *buffer, uint8_t length);
void eeWriteBlock(uint8_t* address, const uint8_t* buffer,
                  uint8_t length);
#endif
