#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <stdint.h>

#include "hardware.h"
#include "uart.h"
#include "bull.h"
#include "morse.h"
#include "ws2812b_led.h"
#include "random.h"
#include "globals.h"

/* This program is written for an Arduino Nano */

unsigned int bufpos;
uint16_t time_ms = 0;
uint32_t time_s = 0;
uint32_t countdown_timer = 0;

// Strings stored in flash
const char strHELLO[] PROGMEM = "HELLO";

void idler(void) {
  // This function is run when uart_getc is idling
  led(morse_getled());
  wdt_reset();
}


int main(void)
{
  uint8_t *c;
  cli();

  wdt_enable(WDTO_8S); // Use a _long_ watchdog timeout.

  initPorts();  // hardware.c
  uart_setup(); // uart.c
  bull_init();  // bull.c
  morse_init();
  initTimers(); //hardware.c
  rnd_init();
  sei(); //Enable interrupts.

  bufpos = 0;

  morse_say_P(strHELLO);
  wsled_color(10,0,0);
  wsled_color(0,10,0);
  wsled_color(0,0,10);

  for (;;) {
    wdt_reset();
    c = uart_getc(500, idler);
    if (c) {
      serialbuffer[bufpos] = *c;
      bufpos++;
      if (is_bull(serialbuffer, bufpos)) {
        handle_bull(serialbuffer, bufpos);
        bufpos = 0;
      } else if (bufpos >= SERIALBUFSIZE) {
        // We are getting a larger message than we can handle.
        // Kill the buffer.
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

  // This interrupt might take a long time and is not _that_ time
  // critical. Reenable interrupts so that other more important
  // interrupts can be run.
  sei();

  PORTB |= (1 << 4); // Debug pin

  time_ms++;
  if (time_ms >= 1000) {
    time_s++;
    time_ms = 0;
  }

  if (time_ms % 100 == 0) {
    morse_tick();
  }

  if (countdown_timer) {
    countdown_timer--;
  }
}
