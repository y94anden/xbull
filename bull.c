#include "bull.h"
#include "uart.h"
#include "hardware.h"
#include "eeprom.h"
#include <stdint.h>

// A bull message consists of the following bytes:
// 0: address
// 1: command
// 2: parameter
// 3: length
// 4-4+length: data
// 4+length: checksum

uint8_t address;
uint8_t bull_inhibit_response;

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
      bull_string_reply(0xFF, 0x00, "Bad checksum");
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
    bull_string_reply(0xFF, 0x00, "Unhandled command");
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
  uart_putc(address);
  sum += address;

  uart_putc(command);
  sum += command;

  uart_putc(param);
  sum += param;

  i = 0;
  while(str[i] != 0 && i < 255) {
    i++;
  }
  uart_putc(i);
  sum += i;

  while(*str) {
    uart_putc(*str);
    sum += (*str);
    str++;
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

void bull_handle_read(uint8_t param, uint8_t len, const uint8_t* data) {
  uint8_t response;
  if (param == 0x01) {
    // Address
    bull_data_reply(0x01, param, 1, &address);
  } else if (param == 0x02) {
    // LED
    response = readled();
    bull_data_reply(0x01, param, 1, &response);
  } else if (param >= 0x10 && param < 0x20) {
    // EEPROM parameters
    response = eeReadByte((uint8_t*)(param - 0x10 + 1));
    bull_data_reply(0x01, param, 1, &response);
  } else {
    // Invalid parameter
    bull_string_reply(0xFF, param, "Invalid parameter");
  }
}

uint8_t bull_verify_length(uint8_t param, uint8_t supplied, uint8_t expected) {
  if (supplied == expected) {
    return 1;
  }
  bull_string_reply(0xFF, param, "Invalid length");
  return 0;
}

void ignore_traffic() {
  // Ignore traffic until we receive no traffic within 5 seconds.
  uint8_t *c = &address; // Initiate with any address.
  while (c) {
    c = uart_getc(5000);
  }
}

void bull_handle_write(uint8_t param, uint8_t len, const uint8_t* data) {
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
  } else if (param >= 0x10 && param < 0x20) {
    // EEPROM parameters
    if(bull_verify_length(param, len, 1)) {
      eeWriteByte((uint8_t*)(param - 0x10 + 1), data[0]);
      bull_data_reply(0x81, param, 0, 0);
    }
  } else {
    // Invalid parameter
    bull_string_reply(0xFF, param, "Invalid parameter");
  }
}
