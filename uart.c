#include <avr/interrupt.h>
#include <util/delay.h>

#include "uart.h"
#include "hardware.h"

#define BAUD 19200

#define RECV_BUFFER_LEN 16
#define SEND_BUFFER_LEN 16

uint8_t rcvBuffer[RECV_BUFFER_LEN];
uint8_t sndBuffer[SEND_BUFFER_LEN];
uint8_t sndHead;
uint8_t sndTail;
uint8_t rcvHead;
uint8_t rcvTail;

extern uint16_t countdown_timer; // From main.c

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
  UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1 << RXCIE0) | (1 << TXCIE0);

  // Set frame format: 8data, 1stop bit
  UCSR0C = (3<<UCSZ00);
}

void uart_putc(uint8_t byte) {
  uint8_t tail;

  // Busy-wait for room in the buffer
  do {
    cli(); // sndTail is moved by interrupt. Disable temporarily.
    tail = sndTail;
    sei();
  } while (sndHead - tail >= SEND_BUFFER_LEN);

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

uint8_t uart_available() {
  uint8_t count;

  cli();
  count = rcvHead - rcvTail;
  sei();

  return count;
}

uint8_t* uart_getc(unsigned int timeout, idler_t idler) {
  uint8_t head;
  uint8_t *p;
  uint8_t done = 0;
  cli();
  head = rcvHead;
  countdown_timer = timeout;
  sei();
  while (head == rcvTail) {
    if (done) {
      // No data in buffer
      return 0;
    }
    if (idler) {
      idler();
    }

    cli();
    head = rcvHead;
    done = (countdown_timer == 0);
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

ISR (USART_TX_vect) {
  // Transmit complete. Time to change directon on RS485?
  if (sndHead == sndTail) {
    // Nothing to send
    // Set RS485 direction back to IN
    rs485_direction_in();
  }
}
