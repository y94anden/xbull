#include <avr/interrupt.h>
#include <util/delay.h>

#include "uart.h"
#include "hardware.h"

#define BAUD 115200

#define RECV_BUFFER_LEN 16
#define SEND_BUFFER_LEN 16

uint8_t rcvBuffer[RECV_BUFFER_LEN];
uint8_t sndBuffer[SEND_BUFFER_LEN];
unsigned int sndHead;
unsigned int sndTail;
unsigned int rcvHead;
unsigned int rcvTail;

void uart_transmit();

void uart_setup() {
  sndHead = 0;
  sndTail = 0;
  rcvHead = 0;
  rcvTail = 0;

  // Set baudrate
  unsigned int ubrr = F_CPU/8/BAUD - 1;
  UBRR0H = ubrr >> 8;
  UBRR0L = ubrr & 0xFF;

  // Set double speed mode
  UCSR0A = (1 << U2X0);

  // Enable receiver and transmitter and interrupts
  UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1 << RXCIE0);

  // Set frame format: 8data, 1stop bit
  UCSR0C = (3<<UCSZ00);
}

void uart_putc(uint8_t byte) {
  unsigned int tail;

  // Busy-wait for room in the buffer
  led(1);
  do {
    cli(); // sndTail is moved by interrupt. Disable temporarily.
    tail = sndTail;
    sei();
  } while (sndHead - tail >= SEND_BUFFER_LEN);
  led(0);

  sndBuffer[sndHead % SEND_BUFFER_LEN] = byte;
  sndHead++;

  if (!(UCSR0B & (1 << UDRIE0))) {
    // Transmit interrupt is not enabled. We must start sending
    cli();
    rs485_direction_out();
    uart_transmit();
    UCSR0B |= (1 << UDRIE0); // Enable transmit interrupt.
    sei();
  }

  // Rewind send pointers to between 0 and SEND_BUFFER_LEN
  while (sndTail >= SEND_BUFFER_LEN) {
    cli();
    sndTail -= SEND_BUFFER_LEN;
    sei();
    sndHead -= SEND_BUFFER_LEN;
  }
}

void uart_puts(const char* str) {
  while (*str) {
    uart_putc(*str);
    str++;
  }
}

unsigned int uart_available() {
  unsigned int count;

  cli();
  count = rcvHead - rcvTail;
  sei();

  return count;
}

uint8_t* uart_getc(unsigned int timeout) {
  unsigned int head;
  uint8_t *p;

  cli();
  head = rcvHead;
  sei();
  while (head == rcvTail) {
    if (timeout == 0) {
      // No data in buffer
      return 0;
    }
    _delay_ms(1);
    timeout--;

    cli();
    head = rcvHead;
    sei();
  }

  p = &rcvBuffer[rcvTail % RECV_BUFFER_LEN];
  rcvTail++;

  while (rcvTail >= RECV_BUFFER_LEN) {
    rcvTail -= RECV_BUFFER_LEN;
    cli(); // Head is moved by interrupt. Disable them
    rcvHead -= RECV_BUFFER_LEN;
    sei();
  }

  return p;
}

void uart_transmit() {
  // Put next byte in UART or stop transmission
  // Called by transmit interrupt or when starting new transmission.

  if (sndHead == sndTail) {
    // Nothing to send
    // Set RS485 direction back to IN
    rs485_direction_in();
    UCSR0B &= ~(1 << UDRIE0); // Disable transmit interrupt.
    return;
  }

  /*
  // Wait for empty transmit buffer
  while (!(UCSR0A & (1<<UDRE0))) {
    ;
  }
  */

  // Put data into buffer, sends the data
  UDR0 = sndBuffer[sndTail % SEND_BUFFER_LEN];

  // Increment buffer pointer
  sndTail++;
}

ISR (USART_RX_vect) {
  if (rcvHead - rcvTail >= RECV_BUFFER_LEN) {
    // overflow! Handle?
    ;
  } else {
    rcvBuffer[rcvHead % RECV_BUFFER_LEN] = UDR0;
    rcvHead++;
  }
}

ISR (USART_UDRE_vect) {
  // A byte was sent. Send another one or set rs485 direction back to in.
  uart_transmit();
}
