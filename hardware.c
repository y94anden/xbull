#include "hardware.h"

#include <util/delay.h>
#include <avr/interrupt.h>

void initPorts() {
  DDRB  = (1 << 5); // Pin 5 output = LED
  PORTB = 0;    // No pullup, output 0

  DDRC = (1 << 5);  // Pin 5 output = WS1812b led chain
  PORTC = 0;    // No pullup

  DDRD  = 0x00; // All inputs
  PORTD = 0;    // No pullup
}

void initTimers() {
  // Setup timer 2 as a 1kHz interrupt source.
  // System clock is 16MHz
  // Prescale with 128 => 125 kHz
  // Have the timer overflow at 125 => 1kHz (clear timer on compare match)
  TCCR2A = (1 << WGM21); // Waveform generation 010=Clear Timer on Compare match)
  TCCR2B = (1 << CS22) | (1 << CS20); // Clock select 101. Prescaler 128.
  OCR2A = 125; // Output compare A.
  TIMSK2 = (1 << OCIE2A); // Output compare interrupt enable, A
}

void clearTimers() {
  TCCR0A = 0;
  TCCR0B = 0;
  TCNT0 = 0;
  OCR0A = 0;
  OCR0B = 0;
  TIMSK0 = 0;
  TIFR0 = 0;

  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1C = 0;
  TCNT1H = 0; // High must be written first
  TCNT1L = 0;
  OCR1AH = 0; // High must be written first
  OCR1AL = 0;
  OCR1BH = 0; // High must be written first
  OCR1BL = 0;
  ICR1H = 0; // High must be written first
  ICR1L = 0;
  TIMSK1 = 0;
  TIFR1 = 0;

  ASSR = 0;
  GTCCR = 0;

  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 0;
  OCR2A = 0;
  OCR2B = 0;
  TIMSK2 = 0;
  TIFR2 = 0;
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
  // Assumptions from the optiboot.c comment
  //     No interrupts can occur
  //     UART and Timer 1 are set to their reset state
  //     SP points to RAMEND

   // Disable interrupts
  cli();

  // Clear UART
  UCSR0A = 0x20;
  UCSR0B = 0;
  UCSR0C = 0x06;
  UBRR0H = 0;
  UBRR0L = 0;

  clearTimers();

  // Move stackpointer to end of RAM
  SP = RAMEND;

  // Clear reset reason
  MCUSR=0;

  do_reboot();
}
