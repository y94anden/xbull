#ifndef BULL_H__
#define BULL_H__

// Parameters
//
// 0x01 Address of unit, R/W
// 0x02 LED control. 0/1 = off/on, >1 = blink that many times, R/W
// 0x03 Ignore traffic. Quiet 5s = start listening again, W
// 0x04 Go into programming mode (optiboot), W
// 0x05 Time in seconds, 32 bit, R/W
// 0x06 Version, R
// 0x07 NeoPixel write, W
// 0x10 |
// ...  | eeprom stored bytes, R/W
// 0x1F |
// 0x20 Reset 1-wire network. Return 1 if anyone responds.
// 0x21 Search next unit on 1-wire network. Supply 1 to start new search. Read
//      To return last found / currently active device id.
// 0x22 Read temperature. Supply device as input, or use last found/read.
// 0x23 Onewire bit, R/W.
void bull_init();
int is_bull(unsigned char* data, unsigned int length);
void handle_bull(unsigned char* data, unsigned int length);

#endif
