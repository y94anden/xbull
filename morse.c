#include "morse.h"
#include <avr/pgmspace.h>

#define SEQUENCE_BUF 32 // Yields about factor 8/9 chars. 32 -> 28 characters.

uint16_t mSequenceLen; // Current length of sequence
uint16_t mSequencePos; // Current bit position in sequence
uint8_t mRepeat;     // Should the sequence be repeted or stopped on last output
uint8_t mSequence[SEQUENCE_BUF];  // Each bit in sequence is led output

void morse_init() {
  morse_clear();
  mRepeat = 1;
}

uint8_t morse_getled() {
  if (mSequencePos >= mSequenceLen) {
    // This is an indicator that we do not want to loop.
    return 0;
  }

  uint8_t b = mSequence[mSequencePos / 8];
  uint8_t bit = mSequencePos % 8;
  uint8_t mask = (1 << (7 - bit));
  return (b & mask) ? 1 : 0;
}

uint8_t morse_tick() {
  if (mSequencePos >= mSequenceLen) {
    // This is an indicator that we do not want to loop.
    return 0;
  }

  uint8_t led = morse_getled();

  mSequencePos++;

  // Have we finished the sequence?
  if (mSequencePos >= mSequenceLen) {
    if (mRepeat) {
      mSequencePos = 0;
    }
  }

  return led;
}

void morse_repeat(uint8_t active) {
  mRepeat = active;

  // Do we need to reset the sequence?
  if (mRepeat) {
    if (mSequencePos >= mSequenceLen) {
      mSequencePos = 0;
    }
  }
}

void morse_restart() {
  mSequencePos = 0;
}

void morse_clear() {
  mSequenceLen = 0;
  mSequencePos = 0;
}

//! Add a bit to the sequence. Return true if successful.
uint8_t morse_add(uint8_t bit) {
  if (mSequenceLen >= (SEQUENCE_BUF*8)) {
    return 0;
  }

  uint8_t currentbit = mSequenceLen % 8;
  uint8_t *currentbyte = &(mSequence[mSequenceLen / 8]);

  if (currentbit == 0) {
    // This is a new byte. Clear it first.
    *currentbyte = 0;
  }

  uint8_t value = (bit ? 1 : 0);
  value <<= (7 - currentbit); // Bit 0 is written as MSB

  *currentbyte |= value;

  mSequenceLen++;

  return 1;
}

uint8_t dot() {
  morse_add(1);
  return morse_add(0);
}

uint8_t dash() {
  morse_add(1);
  morse_add(1);
  morse_add(1);
  return morse_add(0);
}

uint8_t morse_space() {
  morse_add(0);
  return morse_add(0);
}

uint8_t morse_character(char c) {
  // Switch to uppercase character
  switch((c >= 'a' && c <= 'z') ? c - 32 : c) {
  case ' ': morse_space(); break;
  case 'A': dot(); dash(); break;
  case 'B': dash(); dot(); dot(); dot(); break;
  case 'C': dash(); dot(); dash(); dot(); break;
  case 'D': dash(); dot(); dot(); break;
  case 'E': dot(); break;
  case 'F': dot(); dot(); dash(); dot(); break;
  case 'G': dash(); dash(); dot(); break;
  case 'H': dot(); dot(); dot(); dot(); break;
  case 'I': dot(); dot(); break;
  case 'J': dot(); dash(); dash(); dash(); break;
  case 'K': dash(); dot(); dash(); break;
  case 'L': dot(); dash(); dot(); dot(); break;
  case 'M': dash(); dash(); break;
  case 'N': dash(); dot(); break;
  case 'O': dash(); dash(); dash(); break;
  case 'P': dot(); dash(); dash(); dot(); break;
  case 'Q': dash(); dash(); dot(); dash(); break;
  case 'R': dot(); dash(); dot(); break;
  case 'S': dot(); dot(); dot(); break;
  case 'T': dash(); break;
  case 'U': dot(); dot(); dash(); break;
  case 'V': dot(); dot(); dot(); dash(); break;
  case 'W': dot(); dash(); dash(); break;
  case 'X': dash(); dot(); dot(); dash(); break;
  case 'Y': dash(); dot(); dash(); dash(); break;
  case 'Z': dash(); dash(); dot(); dot(); break;

  /* The following characters are multibyte
  case 'ä': // fallthrough
  case 'Ä': dot(); dash(); dot(); dash(); break;
  case 'Á': dot(); dash(); dash(); dot(); dash(); break;
  case 'å': // fallthrough
  case 'Å': dot(); dash(); dash(); dot(); dash(); break;
  case 'Ch': dash(); dash(); dash(); dash(); break;
  case 'É': dot(); dot(); dash(); dot(); dot(); break;
  case 'Ñ': dash(); dash(); dot(); dash(); dash(); break;
  case 'ö': // fallthrough
  case 'Ö': dash(); dash(); dash(); dot(); break;
  case 'Ü': dot(); dot(); dash(); dash(); break;
  */
  case '0': dash(); dash(); dash(); dash(); dash(); break;
  case '1': dot(); dash(); dash(); dash(); dash(); break;
  case '2': dot(); dot(); dash(); dash(); dash(); break;
  case '3': dot(); dot(); dot(); dash(); dash(); break;
  case '4': dot(); dot(); dot(); dot(); dash(); break;
  case '5': dot(); dot(); dot(); dot(); dot(); break;
  case '6': dash(); dot(); dot(); dot(); dot(); break;
  case '7': dash(); dash(); dot(); dot(); dot(); break;
  case '8': dash(); dash(); dash(); dot(); dot(); break;
  case '9': dash(); dash(); dash(); dash(); dot(); break;
  case '.': dot(); dash(); dot(); dash(); dot(); dash(); break;
  case ',': dash(); dash(); dot(); dot(); dash(); dash(); break;
  case ':': dash(); dash(); dash(); dot(); dot(); dot(); break;
  case '`': dot(); dot(); dash(); dash(); dot(); dot(); break;
  case '\'': dot(); dash(); dash(); dash(); dash(); dot(); break;
  case '-': dash(); dot(); dot(); dot(); dot(); dash(); break;
  case '/': dash(); dot(); dot(); dash(); dot(); break;
  case '(': dash(); dot(); dash(); dash(); dot(); dash(); break;
  case '"': dot(); dash(); dot(); dot(); dash(); dot(); break;
  case '@': dot(); dash(); dash(); dot(); dash(); dot(); break;
  case '=': dash(); dot(); dot(); dot(); dash(); break;
    //case 'Error': dot(); dot(); dot(); dot(); dot(); dot(); dot(); dot(); break;
  default:
    /* Unknown character */
    return 0;
  }
  return morse_space(); // Only need to return if last operation was a success.
}

void morse_say_P(const char* string_in_progmem) {
  uint8_t i = 0;
  char c;

  morse_clear();
  c = pgm_read_byte(&string_in_progmem[i]);
  while (c) {
    morse_character(c);
    i++;
    c = pgm_read_byte(&string_in_progmem[i]);
  }
  morse_space(); // Add two extra
  morse_space();
}
