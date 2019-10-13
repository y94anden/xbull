#include "therm_ds18b20.h"

#include <util/delay.h>	//Header for _delay_ms()
#include <avr/interrupt.h>

#define L_ONE ((uint64_t)1)

uint8_t therm_reset() {
  uint8_t i;

  //Pull line low and wait for 480uS
  THERM_OUTPUT_MODE();
  THERM_LOW();
  _delay_us(480);//480 //must be smaller when moving delay func to other .c file

  //Release line and wait for 60uS
  THERM_INPUT_MODE();
  THERM_HIGH(); // Pullup
  _delay_us(70); //60

  //Store line value and wait until the completion of 480uS period
  i=THERM_READ();

  _delay_us(410); //420

  //Return the value read from the presence pulse (0=OK, 1=WRONG)
  return i;
}

void therm_write_bit(uint8_t bit) {
  cli();
  //Pull line low for 6uS
  THERM_OUTPUT_MODE();
  THERM_LOW();
  _delay_us(6);//1

  //If we want to write 1, release the line (if not will keep low)
  if(bit) {
    THERM_INPUT_MODE();
    THERM_HIGH(); // Pullup
  }
  sei();

  //Wait for 54uS and release the line (if not done before)
  _delay_us(54);//60
  THERM_INPUT_MODE();
  THERM_HIGH(); // Pullup
  _delay_us(10);
}

uint8_t therm_read_bit(void) {
  uint8_t bit=0;
  cli();
  //Pull line low for 6uS
  THERM_OUTPUT_MODE();
  THERM_LOW();
  _delay_us(5);//1

  //Release line and wait for 14uS
  THERM_INPUT_MODE();
  THERM_HIGH(); // Pullup
  _delay_us(9);//14

  //Read line value
  if(THERM_READ()) {
    bit=1;
  }

  sei();

  //Wait for 55uS to end and return read value
  _delay_us(55);//45
  return bit;
}

uint8_t therm_read_byte(void) {
  uint8_t i=8, n=0;
  while(i--) {
    //Shift one position right and store read value
    n>>=1;
    n|=(therm_read_bit()<<7);
  }
  return n;
}

void therm_write_byte(uint8_t byte) {
  uint8_t i=8;
  while(i--) {
    //Write actual bit and shift one position right to make the next bit ready
    therm_write_bit(byte&1);
    byte>>=1;
  }
}

void therm_read_temperature(int16_t *temp, uint64_t* id) {
  uint8_t bit;

  //Reset, skip ROM and start temperature conversion
  therm_reset();
  therm_write_byte(THERM_CMD_SKIPROM); //Have all devices to read temp.
  therm_write_byte(THERM_CMD_CONVERTTEMP);

  //Wait until conversion is complete
  while(!therm_read_bit());

  //Reset, skip ROM and send command to read Scratchpad
  therm_reset();
  if (id) {
    // If id is supplied, first match the device using MATCH ROM command
    therm_write_byte(THERM_CMD_MATCHROM);
    for (bit = 0; bit < 64; bit++) {
      therm_write_bit(((*id)>>bit) & 1);
    }
    //therm_write_byte( ((*id)>>0) & 0xFF);
    //therm_write_byte( ((*id)>>8) & 0xFF);
    //therm_write_byte( ((*id)>>16) & 0xFF);
    //therm_write_byte( ((*id)>>24) & 0xFF);
  } else {
    therm_write_byte(THERM_CMD_SKIPROM);
  }
  therm_write_byte(THERM_CMD_RSCRATCHPAD);

  //Read Scratchpad (only 2 first bytes)
  *temp  = therm_read_byte();
  *temp |= (therm_read_byte()<<8);
  therm_reset();

}

uint64_t therm_search(uint64_t* discrepancyMask) {
  //This function will search the network for 1-wire devices.
  //If two devicID's differ at a certain bit, the 1-branch is
  //selected, and the discrepancyMask is set to 1 at this
  //position. If the same discrepancy mask is supplied at the
  //next call, next device is returned.
  //
  //For the first call, set discrepancyMask = 0
  //
  //If the discrepancyMask is 0 after a search command, all
  //devices have been enumerated.
  //
  //The function returns the first deviceID after the last found
  //and > 0xFF000000 if there is an error.
  uint8_t position, normalBit, complementBit, selectedNextBit;
  uint64_t deviceID = 0;

  if(therm_reset()) {
    // No units responding
    return 0xFFFFFFFFFFFFFFFF;
  }

  // Start search
  therm_write_byte(THERM_CMD_SEARCHROM);

  for(position = 0; position < 64; position++) {
    // Read the bit from the devices
    normalBit     = therm_read_bit();
    complementBit = therm_read_bit();

    if (normalBit == 0 && complementBit == 0) {
      //Active devices have different bits in current pos
      //Check if this is the most significant bit in the
      //discrepancyMask. If so, take the other route at this
      //position (zero that is)
      // Ex:
      // 0000100101001 <- least significant
      //     ^ ^  ^  <--- If current position is here:
      //     | |  |
      //     | |  +--We have more to investigate higher up,
      //     | |     select bit = 1
      //     | +-----We have been here already and are now in the
      //     |       bit=0 branch
      //     +-------Time to take the other route (bit = 0). Reset
      //             the mask and go with bit = 0.

      if ( (*discrepancyMask) & (L_ONE << position)) {
        //Devices have different bits, and the mask is 1.
        //If this is the most significant bit in the mask,
        //we should try the other branch now (bit=0). We
        //should also reset the mask here to indicate we
        //are done with the 1-branch.
        //If this is not the most sigificant bit, we need
        //to keep investigating the 1-branch.
        if ( (*discrepancyMask) < (L_ONE << (position+1))) {
          //the mask is less than a 1 in the next position =>
          //this is the most significant bit in the mask.

          //reset the mask
          (*discrepancyMask) ^= (L_ONE << position); //Use XOR - we now it is a one
          selectedNextBit = 0;
        } else {
          //This is not the most significant bit. Keep investigating
          //the 1-branch and leave the mask as is
          selectedNextBit = 1;
        }
      } else {
        //Devices have different bits, but the mask is zero
        //If we have passed the MSB of the mask, we have found
        //a new discrepancy. Set the mask to 1, and go for the
        //1-branch.
        //If we have not passed the MSB, this means we have
        //already investigated the 1-branch from this position and
        //we should keep investigating the 0-branch.

        if ( (*discrepancyMask) < (L_ONE << position)) {
          //the mask is less than a 1 in the current position =>
          //we have passed the MSB of the mask => we have found
          //a new discrepancy
          (*discrepancyMask) |= (L_ONE << position);
          selectedNextBit = 1;
        } else {
          //We have not passed MSB => the 1-branch of this
          //discrepancy have been searched already => keep
          //going to the 0-branch.
          selectedNextBit = 0;
        }
      }
    } else if (normalBit && complementBit) {
      //No good. No device responded.
      return 0xFFFFFFFFFFFFFFFE;
    } else {
      //Bits differed. All active devices had the same bit
      selectedNextBit = normalBit;
    }

    // Update the deviceID with the read/selected bit
    deviceID |= ((uint64_t)selectedNextBit << position);

    // Write the selected bit to continue out in the tree.
    therm_write_bit(selectedNextBit);
  }

  return deviceID;
}
