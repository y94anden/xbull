#include <util/delay.h>
#include <avr/interrupt.h>

#include "hardware.h"
#include "uart.h"

/* This program is written for an Arduino Nano */


int main(void) 
{
  unsigned char *c;

  cli();
  
  initPorts();  // hardware.c
  uart_setup(); // uart.c

  sei(); //Enable interrupts.

  for (;;) {
    c = uart_getc();
    if (c) {
      uart_putc(*c);
      uart_putc(*c ^ 0x20); // Toggle case
    }
  }

  return 0;
}
