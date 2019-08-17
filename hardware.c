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

uint8_t readled() {
  return (PORTB &= (1 << 5)) >> 5;
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


// Define a function that points to the bootloader location. The address
// is hardcoded for a 512 byte bootloader.
typedef void (*do_reboot_t)(void);
const do_reboot_t do_reboot = (do_reboot_t)((FLASHEND-511)>>1);

void programming_mode() {
  // Turn off everything and jump to bootloader address
  cli(); // Disable interrupts
  // TOOD: Perhaps we need to reset some timer registers to default values?

  MCUSR=0;
  do_reboot();
}
