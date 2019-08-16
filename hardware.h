#ifndef HARDWARE_H_
#define HARDWARE_H_

//#define F_CPU 16000000UL  /* Defined in Makefile */
#include <avr/io.h>
#include <stdint.h>

void initPorts();
void led(int on);
uint8_t readled();
void blink(unsigned int count);

// Set direction pin for RS485 transciever
void rs485_direction_out();
void rs485_direction_in();
int rs485_is_sending();
#endif
