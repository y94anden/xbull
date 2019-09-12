PROJECT=xbull

MCU=atmega328p
F_CPU=16000000
PORT=/dev/ttyUSB0

SRCS = main.c \
       hardware.c \
       uart.c \
       bull.c \
       eeprom.c \
       morse.c \
       ws2812b_led.c \
       therm_ds18b20.c \
       random.c \
       sha256.c \
       search.c \
       globals.c

.PHONY: all
all: $(PROJECT).hex

CC=avr-gcc
CFLAGS=-Wall -Werror -Os -mmcu=${MCU} -DF_CPU=${F_CPU}

OBJECTS := $(SRCS:%.c=%.o)

# Mucking about with auto dependencies
DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d
COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

$(OBJECTS): | $(DEPDIR)

%.o : %.c $(DEPDIR)/%.d
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(DEPDIR): ; @mkdir -p $@

DEPFILES := $(SRCS:%.c=$(DEPDIR)/%.d)
$(DEPFILES):

include $(wildcard $(DEPFILES))


# end autodependencies...


AVRDUDE = avrdude -p m328p -c arduino -P ${PORT}
AVRDUDE_ISP = avrdude -p m328p -c usbasp

$(PROJECT).hex: $(PROJECT).elf
	avr-objcopy -O ihex  -R .eeprom $< $@

# When make needs to link the binary, we sneak in an extra object
# containing versioning information. This file will only be created
# if something else was rebuilt.
$(PROJECT).elf: $(OBJECTS)
	./setversion.sh
	@$(CC) $(CFLAGS) -c version.c -o version.o
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) version.o $(LDFLAGS)

.PHONY: clean
clean:
	rm -rf $(DEPDIR)
	rm -f *.o *.hex *.elf version.c

.PHONY: erase
erase:
	$(AVRDUDE_ISP)  -e

flash: $(PROJECT).hex
	$(AVRDUDE) -U $(PROJECT).hex
	touch flash

flash_boot: optiboot_atmega328.hex
	$(AVRDUDE_ISP) -U optiboot_atmega328.hex
	touch flash_boot

.PHONY: terminal
terminal:
	$(AVRDUDE) -t

.PHONY: disasm
disasm: $(PROJECT).elf
	avr-objdump -d $<


# Atmega328p fuses
#
# Fuse high byte
# 7 6 5 4 3 2 1 0
# 1 1 0 1 1 1 1 0  =  0xde
# ^ ^ ^ ^ ^ \+/ ^
# | | | | |  |  |
# | | | | |  |  +----- BOOTRST  (Select reset vector)
# | | | | |  +-------- BOOTSZ   (Boot size. 00=big, 11=small)
# | | | | +----------- EESAVE   (EEProm preserved through chip erase)
# | | | +------------- WDTON    (Watchdog always on)
# | | +--------------- SPIEN    (SPI Enable - serial programming)
# | +----------------- DWEN     (debugWire enable)
# +------------------- RSTDISBL (Reset disable)


# Fuse low byte
# 7 6 5 4 3 2 1 0
# 1 1 1 1 0 1 1 1  =  0xf7
# ^ ^ \+/ \--+--/
# | |  |     |
# | |  |     +-------- CKSEL    (Clock source select)
# | |  +-------------- SUT      (Select startup time)
# | +----------------- CKOUT    (Clock output)
# +------------------- CKDIV8   (Divide clock by 8)
#

# Fuse extended byte
# 7 6 5 4 3 2 1 0
# 1 1 1 1 1 1 0 1  =  0xfd
#           \-+-/
#             |
#             +------- BODLEVEL


.PHONY: fuses
fuses:
	$(AVRDUDE_ISP) -U hfuse:w:0xde:m -U lfuse:w:0xf7:m -U efuse:w:0xfd:m

.PHONY: readcal
readcal:
	$(AVRDUDE_ISP) -U calibration:r:/dev/stdout:i | head -1

.PHONY: size
size: $(PROJECT).elf
	avr-size $<
