#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "hardware.h"
#include "uart.h"
#include "bull.h"

/* This program is written for an Arduino Nano */

#define BUFSIZE 32
uint8_t buffer[BUFSIZE];
unsigned int bufpos;

int main(void)
{
  uint8_t *c;
  cli();

  initPorts();  // hardware.c
  uart_setup(); // uart.c
  bull_init();  // bull.c

  sei(); //Enable interrupts.

  bufpos = 0;
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
