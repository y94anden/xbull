#ifndef UART_H__
#define UART_H_

#include <stdint.h>

// Type of idler function that is called while waiting for uart receive timeout
typedef void (*idler_t)(void);

// Setup port with interrupts, baudrate etc
void uart_setup();

// Queue one byte for sending. Will block until there is room in the buffer.
void uart_putc(uint8_t);

// Return number of bytes waiting in receive buffer
uint8_t uart_available();

// Return pointer to next character in receive buffer. Returns NULL if no
// data is is available within timeout ms.
uint8_t* uart_getc(unsigned int timeout, idler_t idler);

// Send nullterminated string.
void uart_puts(const char* str);

#endif
