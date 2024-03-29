#ifndef BULL_H__
#define BULL_H__

// Parameters
//
// 0x01 Address of unit. Read with any payload blink id on led. R/W
// 0x02 Name of unit, up to 16 bytes, R/W
// 0x03 Ignore traffic. Quiet 5s = start listening again, W
// 0x04 Go into programming mode (optiboot), W
// 0x05 Time in seconds, 32 bit, R/W
// 0x06 Version, R
// 0x07 NeoPixel write, W
// 0x08 Search xBull units, R/W (see search.h for explanation)
// 0x09 Read flash page, R.
// 0x0A Read chip info, R. Fuses(L, H, E, lock), Signature, Calibration
// 0x0B SPI, at most 4 bytes, R/W (Read to reenable LED and disable SPI)
// 0x10 |
// ...  | eeprom stored bytes, R/W
// 0x1F |
// 0x20 Reset 1-wire network. Return 1 if anyone responds.
// 0x21 Search next unit on 1-wire network. Supply 1 to start new search. Read
//      To return last found / currently active device id.
// 0x22 Read temperature. Input: device, 0x01 for search next after reply or
//      nothing to return same as last.
// 0x23 Onewire bit, R/W.
// 0x24 Read DHT11, R.
void bull_init();
int is_bull(unsigned char* data, unsigned int length);
void handle_bull(unsigned char* data, unsigned int length);
void ignore_traffic();
#endif
