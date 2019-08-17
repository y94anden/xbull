#ifndef MORSE_H__
#define MORSE_H__

#include <stdint.h>

void morse_init();
uint8_t morse_getled();
uint8_t morse_tick();
void morse_repeat(uint8_t active);
void morse_restart();
void morse_clear();
uint8_t morse_add(uint8_t bit);
uint8_t dot();
uint8_t dash();
uint8_t morse_space();
uint8_t morse_character(char c);
void morse_say_P(const char* string_in_progmem);
#endif // MORSE_H__
