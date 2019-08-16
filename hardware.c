#include "hardware.h"

#include <util/delay.h>

void initPorts()
{
  DDRB  = (1 << 5); // Pin 5 output = LED
  PORTB = 0;    // No pullup, output 0

  DDRC  = 0x00; // All inputs
  PORTC = 0;    // No pullup

  DDRD  = 0x00; // All inputs
  PORTD = 0;    // No pullup
}

void led(int on) {
  if (on) {
    PORTB |= (1 << 5);
  } else {
    PORTB &= ~(1 << 5);
  }
}

void blink(unsigned int count) {
  while (count > 0) {
    led(1);
    _delay_ms(50);
    led(0);
    _delay_ms(50);
    count--;
  }
}


void rs485_direction_out(){
  // TODO: Set direction pin
}

void rs485_direction_in(){
  // TODO: Set direction pin
}

int rs485_is_sending() {
  // TODO: Return true if direction pin is out
  return 0;
}
