#include <avr/io.h>
#include <stdio.h>
#define THERM_PORT PORTC
#define THERM_DDR DDRC
#define THERM_PIN PINC
#define THERM_DQI PC0

/* Utils */
#define THERM_INPUT_MODE() THERM_DDR&=~(1<<THERM_DQI)
#define THERM_OUTPUT_MODE() THERM_DDR|=(1<<THERM_DQI)
#define THERM_LOW() THERM_PORT&=~(1<<THERM_DQI)
#define THERM_HIGH() THERM_PORT|=(1<<THERM_DQI)
#define THERM_READ() (THERM_PIN & (1 << THERM_DQI))

/* list of these commands translated into C defines:*/
#define THERM_CMD_CONVERTTEMP 0x44
#define THERM_CMD_RSCRATCHPAD 0xbe
#define THERM_CMD_WSCRATCHPAD 0x4e
#define THERM_CMD_CPYSCRATCHPAD 0x48
#define THERM_CMD_RECEEPROM 0xb8
#define THERM_CMD_RPWRSUPPLY 0xb4
#define THERM_CMD_SEARCHROM 0xf0
#define THERM_CMD_READROM 0x33
#define THERM_CMD_MATCHROM 0x55
#define THERM_CMD_SKIPROM 0xcc
#define THERM_CMD_ALARMSEARCH 0xec
/* constants */
#define THERM_DECIMAL_STEPS_12BIT 625 //.0625



void therm_delay(uint16_t delay);
uint8_t therm_reset();
void therm_write_bit(uint8_t bit);
uint8_t therm_read_bit(void);
uint8_t therm_read_byte(void);
void therm_write_byte(uint8_t byte);
uint64_t therm_search(uint64_t* dicrepancyMask);
//
void therm_read_temperature(int16_t* temp, uint64_t* id);
