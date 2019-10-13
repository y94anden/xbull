#include "hardware.h"

#include <util/delay.h>
#include <avr/interrupt.h>

// Used pins:
//
// PB2 /SS for SPI. Must not be low during SPI operations.
// PB3 MOSI
// PB4 MISO
// PB5 onboard LED (or SCK for SPI)
//
// PC0 onewire (see thermds18b20.h)
// PC1 Grounding button and source for random bit using ADC.
// PC4 DHT22
// PC5 WS1812b led chain
//
// PD2 RS485 direction pin
// PD4 Debug pin



void initPorts() {
  DDRB  = 0;
  DDRB |= (1 << 2);  // Pin 2 output = SPI /Slave Seleect
  DDRB |= (1 << 3);  // Pin 3 output = MOSI
  DDRB |= (1 << 5);  // Pin 5 output = LED, SPI SCK
  PORTB = 0;         // No pullup, output 0
  PORTB |= (1 << 2); // Pin 2, output high

  DDRC = (1 << 5);   // Pin 5 output = WS1812b led chain
  PORTC = (1 << 1);  // Pin 1 pullup = button / random ADC.
  PORTC |= (1 << 4); // Pin 4 pullup = DHT 11

  DDRD = 0;
  DDRD |= (1 << 2);  // Pin 2 output = RS485 direction
  DDRD |= (1 << 4);  // Pin 4 output = debug
  PORTD = 0;         // No pullup.
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
  PORTD |= (1 << 2);
}

void rs485_direction_in(){
  PORTD &= ~(1 << 2);
}

uint16_t read_adc1() {
  // ADC multiplexer selection register
  ADMUX =
    (1 << REFS0) | // REFS = 1 => AVcc as voltage reference
    (1 << 0);      // MUX = 1  => ADC1

  // ADC control and status register a
  ADCSRA =
    (1 << ADEN) | // ADC Enable
    (7 << 0) |    // Prescaler 128
    (1 << ADSC);  // ADC start conversion

  while(!(ADCSRA & (1 << ADIF))) {
    ; // Busy wait for conversion to finish
  }

  uint8_t temp;
  uint16_t result;
  temp = ADCL;
  result = ADCH;
  result <<= 8;
  result |= temp;
  return result;
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

void spi_enable() {
  // CPOL = 0, Clock low when inactive
  // CPHA = 0, Sample on leading edge (= rising)
  // DORD = 0, Data order = 0, MSB first
  SPCR =
    (1 << SPIE) | // SPI Interrupt Enable
    (1 << SPE) |  // SPI Enable
    (1 << SPR0) | // Rate: 0=f/4, 1=f/16, 2=f/64, 3=f/128
    (1 << MSTR); // SPI Master
}

void spi_disable() {
  SPCR = 0;
}

void dht_pin_low() {
  DDRC |= (1 << 4);
  PORTC &= (~(1 << 4));
}

void dht_pin_input() {
  DDRC &= (~(1 << 4));
  PORTC |= (1 << 4);
}

uint8_t dht_pin() {
  return (PINC & (1 << 4)) >> 4;
}
