#include "ws2812b_led.h"
#include "hardware.h"
#include <avr/interrupt.h>

/*
Module to handle adressable leds

Data is serial, with 24 bits RGB for the first led
followed by 24 bits RGB for the next led etc.

When done, serial line is held low a bit longer making
the latched values "bite".

Each bit is a 1 followed by a 0. The length of the 1
determines the value of the bit.

Between each bit is a 0.

Frequency of the pulse train is 800kHz.

Timing:                                 Min     Typ     Max
T0H     0 code ,high voltage time       200     350     500     ns
T1H     1 code ,high voltage time       550     700     5500-   ns
TLD     data, low voltage time          450     600     5000    ns
TLL     latch, low voltage time         6000                    ns

When waiting for a reset, the line can be held high. The leds can apparently
wait indefinitely in this state. After the reset, keep the line low.

A 16MHz clock needs 4 cycles for 250ns, and 8 cycles for 500ns. 3 cycles yields
only 187.5 ns which is less than the shortest zero bit high part.

So:
T0H = 6 cycles  = 375 ns
T1H = 10 cycles = 625 ns
T1L = 10 cycles = 625 ns


# Using interrupts

This was a previous attempt at using interrupts to signal the LED. That has
beed decided against to avoid messing with other functions.

Run fast PWM in non inverting mode (1 at bottom, clear at compare match, reset
at top) with TOP = 20 and compare match = 6 for zero and 10 for one.

Trig an interrupt at compare match and update the value for the next compare.
Since the writing of the output compare register is buffered and not updated
until the timer wraps, there should not be a timing issue.

The TOP value can be increased to something less than 80 = 5000 ns if more time
for calculations are needed.
*/

void wsled_sendBitZero() {
  // 1. Send a one for 200-500ns = 4-8 clock cycles
  // 2. Send a zero for 450-5500 ns = 8-88 clock cycles.
  //
  // For 1. we cannot allow interrupts, but for 2. we should make sure that
  // we spend at least 8 cycles, but with interrupts enabled. Note that the
  // calling function will use some clock cycles for looping etc as well.

  // PortC is register $08
  // Pin 5 is supposed to be connected to ws2812b.
  asm volatile("cli"           "\n\t"
               "sbi 8,5"       "\n\t"
               "nop"           "\n\t"
               "nop"           "\n\t"
               "cbi 8,5"       "\n\t"
               "sei"           "\n\t"
               "nop"           "\n\t"
               "nop"           "\n\t"
               "nop"           "\n\t"
               ::);
}

void wsled_sendBitOne() {
  // 1. Send a one for at least 550 ns = 9 clock cycles
  // 2. Send a zero for 450-5500 ns = 8-88 clock cycles.
  //

  // PortB is register $05
  // Pin 6 is supposed to be connected to ws2812b.
  asm volatile("sbi 8, 5"      "\n\t"
               "nop"           "\n\t"
               "nop"           "\n\t"
               "nop"           "\n\t"
               "nop"           "\n\t"
               "nop"           "\n\t"
               "nop"           "\n\t"
               "nop"           "\n\t"
               "nop"           "\n\t"
               "nop"           "\n\t"
               "cbi 8,5"       "\n\t"
               "nop"           "\n\t"
               "nop"           "\n\t"
               "nop"           "\n\t"
               "nop"           "\n\t"
               ::);
}

void wsled_sendByte(uint8_t byte) {
  uint8_t i;
  // Bits should be sent with high bit first.
  for (i = 0; i < 8; i++) {
    if(byte & 0x80) {
      wsled_sendBitOne();
    } else {
      wsled_sendBitZero();
    }
    byte <<= 1;
  }
}

void wsled_color(uint8_t r, uint8_t g, uint8_t b) {
  // Neopixel wants them green, red, blue.
  wsled_sendByte(g);
  wsled_sendByte(r);
  wsled_sendByte(b);
}

