#ifndef UART_H__
#define UART_H__

// Setup port with interrupts, baudrate etc
void uart_setup();

// Queue one byte for sending. Will block until there is room in the buffer.
void uart_putc(unsigned char);

// Return number of bytes waiting in receive buffer
unsigned int uart_available();

// Return pointer to next character in receive buffer. Returns NULL if no
// data is is available within timeout ms.
unsigned char* uart_getc(unsigned int timeout);

// Send nullterminated string.
void uart_puts(const char* str);

#endif
