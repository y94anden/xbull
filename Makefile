PROJECT=xbull


MCU=atmega328p
F_CPU=16000000
PORT=/dev/ttyUSB0

SRCS = main.c \
       hardware.c \
       uart.c \
       bull.c \
       eeprom.c \
       morse.c

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


$(PROJECT).hex: $(PROJECT).elf
	avr-objcopy -O ihex  -R .eeprom $< $@

# When make needs to link the binary, we sneak in an extra object
# containing versioning information. This file will only be created
# if something else was rebuilt.
$(PROJECT).elf: $(OBJECTS)
	echo "Buildning new version file"
	@echo "#include <avr/pgmspace.h>" > ver.c
	@echo -n "const char strVERSION[] PROGMEM = \"" >> ver.c
	@date +"%Y-%m-%d %H:%M:%S " | tr -d "\n" >> ver.c
	@git describe --match=NeVeRmAtCh --always --dirty | tr -d "\n" >> ver.c
	@echo '";' >> ver.c
	@$(CC) $(CFLAGS) -c ver.c -o ver.o
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) ver.o $(LDFLAGS)

clean:
	rm -rf $(DEPDIR)
	rm -f *.o *.hex *.elf ver.c

erase:
	$(AVRDUDE)  -e

flash: $(PROJECT).hex
	$(AVRDUDE) -U $(PROJECT).hex
	touch flash

terminal:
	$(AVRDUDE) -t

disasm: $(PROJECT).elf
	avr-objdump -d $<


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
	avr-size $<
