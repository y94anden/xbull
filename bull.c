#include "bull.h"
#include "uart.h"
#include "hardware.h"
#include "eeprom.h"
#include "morse.h"
#include "version.h"
#include "ws2812b_led.h"
#include "therm_ds18b20.h"
#include <stdint.h>
#include <avr/pgmspace.h>

// A bull message consists of the following bytes:
// 0: address
// 1: command
// 2: parameter
// 3: length
// 4-4+length: data
// 4+length: checksum

uint8_t address;
uint8_t bull_inhibit_response;
uint64_t therm_discrepancy_mask;
uint64_t therm_device_id;

extern uint32_t time_s; // Defined in main.c
void idler(void);       // Defined in main.c

// Strings stored in flash
const char strOK[]                    PROGMEM = "OK";
const char strDEAF[]                  PROGMEM = "Deaf";
const char strLISTENING[]             PROGMEM = "Listening";
const char strINVALID_PARAMETER[]     PROGMEM = "Invalid parameter";
const char strBAD_CHECKSUM[]          PROGMEM = "Bad checksum";
const char strUNHANDLED_COMMAND[]     PROGMEM = "Unhandled command";
const char strINVALID_LENGTH[]        PROGMEM = "Invalid length";
const char strPROGRAMMING_MODE_FAIL[] PROGMEM = "Failed programming mode";
const char strLENGTH_MULTIPLE_OF_THREE[] PROGMEM =
  "Length must be a multiple of three";

int checksum_ok(uint8_t* data, unsigned int length);
void bull_string_reply(uint8_t command, uint8_t param, const char* str);
void bull_data_reply(uint8_t command, uint8_t param, uint8_t len,
                     const uint8_t* data);
void bull_handle_read(uint8_t param, uint8_t len, const uint8_t* data);
void bull_handle_write(uint8_t param, uint8_t len, const uint8_t* data);

void bull_init() {
  address = eeReadByte(0);
  if (address == 0xFF) {
    // When eeprom is cleared, it reads as FF. Set our address to 0 if so.
    address = 0;
  }
}

int is_bull(uint8_t* data, unsigned int length) {
  if (length < 5) {
    return 0;
  }
  if (length >= data[3] + 5) {
    // The length is correct
    return 1;
  }
  return 0;

}

void handle_bull(uint8_t* data, unsigned int length) {
  if (data[0] != address && data[0] != 0xFF) {
    // This is not our addres and not a broadcast message
    return;
  }

  if (!checksum_ok(data, length)) {
    if (data[0] == address) {
      // This is for us, and we are expected to answer something. Error.
      bull_string_reply(0xFF, 0x00, strBAD_CHECKSUM);
    }
    return;
  }

  // We do not want to respond to broadcast
  bull_inhibit_response = (data[0] == 0xFF);

  // Check the command
  switch(data[1]) {
  case 0x01: // read
    bull_handle_read(data[2], data[3], &data[4]);
    break;
  case 0x81: // write
    bull_handle_write(data[2], data[3], &data[4]);
    break;
  default:
    bull_string_reply(0xFF, 0x00, strUNHANDLED_COMMAND);
  }
}

int checksum_ok(uint8_t* data, unsigned int length) {
  unsigned int i = 0;
  uint8_t sum = 0;
  for (i = 0; i < length - 1; i++) {
    sum += data[i]; // Sum with overflow
  }
  return sum == data[length-1];
}

void bull_string_reply(uint8_t command, uint8_t param, const char* str) {
  if (bull_inhibit_response) {
    // We do not want to respond to broadcasts
    return;
  }
  unsigned int i;
  uint8_t sum = 0;
  char c;
  uart_putc(address);
  sum += address;

  uart_putc(command);
  sum += command;

  uart_putc(param);
  sum += param;

  i = 0;
  // Calculate length from progmem
  while(pgm_read_byte(&(str[i])) != 0 && i < 255) {
    i++;
  }
  uart_putc(i);
  sum += i;

  // Add data from progmem
  i = 0;
  c = pgm_read_byte(&(str[i]));
  while(c) {
    uart_putc(c);
    sum += (c);
    i++;
    c = pgm_read_byte(&(str[i]));
  }

  uart_putc(sum);
}

void bull_data_reply(uint8_t command, uint8_t param, uint8_t len,
                     const uint8_t* data) {
  if (bull_inhibit_response) {
    // We do not want to respond to broadcasts
    return;
  }
  unsigned int i;
  uint8_t sum = 0;
  uart_putc(address);
  sum += address;

  uart_putc(command);
  sum += command;

  uart_putc(param);
  sum += param;

  uart_putc(len);
  sum += len;

  for (i = 0; i < len; i++) {
    uart_putc(data[i]);
    sum += data[i];
  }

  uart_putc(sum);
}

void bull_version_reply() {
  if (bull_inhibit_response) {
    // We do not want to respond to broadcasts
    return;
  }
  uint8_t i;
  uint8_t length;
  uint8_t sum = 0;
  uint8_t c;
  uart_putc(address);
  sum += address;

  uart_putc(0x01); // Command = read
  sum += 0x01;

  uart_putc(0x06); // Parameter = 0x06
  sum += 0x06;

  length = version_length();
  uart_putc(length);
  sum += length;

  for (i = 0; i < length; i++) {
    c = version_char(i);
    uart_putc(c);
    sum += c;
  }

  uart_putc(sum);

}

void bull_handle_read(uint8_t param, uint8_t len, const uint8_t* data) {
  uint8_t response;
  if (param == 0x01) {
    // Address
    bull_data_reply(0x01, param, 1, &address);
  } else if (param == 0x02) {
    // LED
    response = readled();
    bull_data_reply(0x01, param, 1, &response);
  } else if (param == 0x05) {
    // Time
    bull_data_reply(0x01, param, 4, (uint8_t*)&time_s);
  } else if (param == 0x06) {
    // Version
    bull_version_reply();
  } else if (param >= 0x10 && param < 0x20) {
    // EEPROM parameters
    response = eeReadByte((uint8_t*)(param - 0x10 + 1));
    bull_data_reply(0x01, param, 1, &response);
  } else if (param == 0x22) {
    // Read DS18B20 temperature
    if (len == 8) {
      therm_device_id = *((uint64_t*)data);
    }
    int16_t temperature;
    therm_read_temperature(&temperature, 0);
    bull_data_reply(0x01, param, 2, (uint8_t*)&temperature);
  } else {
    // Invalid parameter
    bull_string_reply(0xFF, param, strINVALID_PARAMETER);
  }
}

uint8_t bull_verify_length(uint8_t param, uint8_t supplied, uint8_t expected) {
  if (supplied == expected) {
    return 1;
  }
  bull_string_reply(0xFF, param, strINVALID_LENGTH);
  return 0;
}

void ignore_traffic() {
  // Ignore traffic until we receive no traffic within 5 seconds.
  morse_say_P(strDEAF);
  uint8_t *c = &address; // Initiate with any address.
  while (c) {
    c = uart_getc(5000, idler);
  }
  morse_say_P(strLISTENING);
}

void bull_handle_write(uint8_t param, uint8_t len, const uint8_t* data) {
  uint16_t i;
  if (param == 0x01) {
    // Address
    if (bull_verify_length(param, len, 1)) {
      address = data[0];
      eeWriteByte((uint8_t*)0, address);
      bull_data_reply(0x81, param, 0, 0);
    }
  } else if (param == 0x02) {
    // LED
    if (bull_verify_length(param, len, 1)) {
      bull_data_reply(0x81, param, 0, 0);
      if (data[0] <= 1) {
        led(data[0]);
      } else {
        blink(data[0]);
      }
    }
  } else if (param == 0x03) {
    // Ignore traffic until quiet for 5 seconds. If payload data is my address,
    // keep listening. This is used to be able to send any binary data to one
    // device while all others ignore it, such as during an upgrade.
    if (bull_verify_length(param, len, 1)) {
      if (data[0] == address || data[0] == 0xFF) {
        // Even if this was a broadcast, we should respond. It was for us.
        // It can also be for everyone to respond. That would only work with
        // a single device.
        bull_inhibit_response = 0;
      } else {
        // Not for us. Stop processing incoming traffic.
        ignore_traffic();
      }
    }
    bull_data_reply(0x81, param, 0, 0);
  } else if (param == 0x04) {
    // Go into programming mode. All normal execution stops.
    programming_mode();
    bull_string_reply(0xFF, param, strPROGRAMMING_MODE_FAIL);
  } else if (param == 0x05) {
    // Time
    if (bull_verify_length(param, len, 4)) {
      time_s = *((uint32_t*)(data)); // Cast the four bytes to an int.
      bull_data_reply(0x81, param, 0, 0);
    }
  } else if (param == 0x07) {
    // NeoPixel write
    if(len == 0 || (len % 3) != 0) {
      bull_string_reply(0xFF, param, strLENGTH_MULTIPLE_OF_THREE);
      return;
    }
    for(i = 0; i < len; i += 3) {
      wsled_color(data[i], data[i+1], data[i+2]);
    }
    bull_string_reply(0x81, param, strOK);
  } else if (param >= 0x10 && param < 0x20) {
    // EEPROM parameters
    if(bull_verify_length(param, len, 1)) {
      eeWriteByte((uint8_t*)(param - 0x10 + 1), data[0]);
      bull_data_reply(0x81, param, 0, 0);
    }
  } else if (param == 0x20) {
    // 1-wire reset
    uint8_t r = therm_reset();
    bull_data_reply(0x81, 0x20, 1, &r);
  } else if (param == 0x21) {
    // 1-wire search next. Supply nonzero data[0] to start new search.
    if (len == 1 && data[0]) {
      therm_discrepancy_mask=0;
    }
    therm_device_id = therm_search(&therm_discrepancy_mask);
    bull_data_reply(0x81, 0x20, 8, (uint8_t*)&therm_device_id);
  } else {
    // Invalid parameter
    bull_string_reply(0xFF, param, strINVALID_PARAMETER);
  }
}
