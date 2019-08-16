#ifndef UART_H__
#define UART_H_

#include <stdint.h>

// Setup port with interrupts, baudrate etc
void uart_setup();

// Queue one byte for sending. Will block until there is room in the buffer.
void uart_putc(uint8_t);

// Return number of bytes waiting in receive buffer
unsigned int uart_available();

// Return pointer to next character in receive buffer. Returns NULL if no
// data is is available within timeout ms.
uint8_t* uart_getc(unsigned int timeout);

// Send nullterminated string.
void uart_puts(const char* str);

#endif
