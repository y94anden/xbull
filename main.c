#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>

#include "hardware.h"
#include "uart.h"
#include "bull.h"
#include "morse.h"

/* This program is written for an Arduino Nano */

#define BUFSIZE 32
uint8_t buffer[BUFSIZE];
unsigned int bufpos;
uint16_t time_ms = 0;
uint32_t time_s = 0;


// Strings stored in flash
const char strHELLO[] PROGMEM = "HELLO";

int main(void)
{
  uint8_t *c;
  cli();

  initPorts();  // hardware.c
  uart_setup(); // uart.c
  bull_init();  // bull.c
  morse_init();
  initTimers(); //hardware.c
  sei(); //Enable interrupts.

  bufpos = 0;

  morse_say(strHELLO);
  for (;;) {
    c = uart_getc(500);
    if (c) {
      buffer[bufpos] = *c;
      bufpos++;
      if (is_bull(buffer, bufpos)) {
        handle_bull(buffer, bufpos);
        bufpos = 0;
      }
    } else {
      // No more data. Clear buffer.
      bufpos = 0;
    }
  }
}

ISR(TIMER2_COMPA_vect) {
  // Called with 1kHz
  time_ms++;
  if (time_ms >= 1000) {
    time_s++;
    time_ms = 0;
  }

  if (time_ms % 100 == 0) {
    led(morse_tick());
  }
}
