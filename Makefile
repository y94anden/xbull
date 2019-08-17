PROJECT=xbull


MCU=atmega328p
F_CPU=16000000
PORT=/dev/ttyUSB0


CC=avr-gcc
CFLAGS=-Wall -Werror -Os -mmcu=${MCU} -DF_CPU=${F_CPU}


all: $(PROJECT).hex


OBJECTS = \
	main.o \
	hardware.o \
	uart.o \
	bull.o \
	eeprom.o


AVRDUDE = avrdude -p m328p -c arduino -P ${PORT}


$(PROJECT).hex: $(PROJECT).elf
	avr-objcopy -O ihex  -R .eeprom $(PROJECT).elf $(PROJECT).hex

$(PROJECT).elf: $(OBJECTS)
	$(CC) $(CFLAGS) -o $(PROJECT).elf $(OBJECTS)



clean:
	rm -f *.o *.hex *.elf

erase:
	$(AVRDUDE)  -e

flash: $(PROJECT).hex
	$(AVRDUDE) -U $(PROJECT).hex
	touch flash

terminal:
	$(AVRDUDE) -t

disasm: $(PROJECT).elf
	avr-objdump -d $(PROJECT).elf


# Atmega8 fuses
#
# Fuse high byte
# 7 6 5 4 3 2 1 0
# 1 1 0 0 1 1 1 1  =  0xCF
# ^ ^ ^ ^ ^ \+/ ^
# | | | | |  |  |
# | | | | |  |  +----- BOOTRST  (Select reset vector)
# | | | | |  +-------- BOOTSZ   (Boot size. 00=small, 11=big)
# | | | | +----------- EESAVE   (EEProm preserved through chip erase)
# | | | +------------- CKOPT    (Oscillator options. 0 = rail_to_rail)
# | | +--------------- SPIEN    (SPI Enable - serial programming)
# | +----------------- WDTON    (Watchdog always on)
# +------------------- RSTDISBL (Reset disable)


# Fuse low byte
# 7 6 5 4 3 2 1 0
# 0 1 1 1 1 1 1 1  =  0x7F
# ^ ^ \+/ \--+--/
# | |  |     |
# | |  |     +-------- CKSEL    (Clock source select)
# | |  +-------------- SUT      (Select startup time)
# | +----------------- BODEN    (Brown out detector enable)
# +------------------- BODLEVEL (Brown out detector level. 0 on atmega8)
#

fuses:
	$(AVRDUDE) -U hfuse:w:0xcf:m -U lfuse:w:0x7f:m

readcal:
	$(AVRDUDE) -U calibration:r:/dev/stdout:i | head -1

size: $(PROJECT).elf
	avr-size $(PROJECT).elf
