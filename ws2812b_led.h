#ifndef WS2801_LED_H__
#define WS2801_LED_H__

#include <stdint.h>


// Add the color of the next led in line. Returns when bits have been sent.
// Each call leaves the output low, meaning that if the function is not
// called within 60µs, the leds consider this a reset.
void wsled_color(uint8_t r, uint8_t g, uint8_t b);


#endif
