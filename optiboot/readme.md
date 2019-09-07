# optiboot

This is the bootloader running in the AVR. It is stolen from
official repos, but with RS485 support added.

# Building

Clone https://github.com/Optiboot/optiboot.git and apply the
`0001-Added-support-for-RS485.patch` to commit
`b8b760546b6f0553a02ff689d57eaa29666e871a` using command
`git am < 0001-Added-support-for-RS485.patch`.

Type `make help` in `optiboot/bootloader/optiboot` to see a
list of parameters to set. For an Arduino Nano based on an
AtMega328p, the following build command can be used:
```
make RS485=D2 CUSTOM_VERSION=100 atmega328
```
This will build a version with RS485 direction pin output at
pin D2, ie PORTD, pin 2.

The `CUSTOM_VERSION` is used to distinguish betwen the official
versions and homebuilt ones.

# Prebuild binary

The supplied binary is built for an atmega328 with RS485 pin at
PD2.

# Flashing

Using an ISP programmer of type `usbasp`, the following comand will
do the trick:
```
avrdude -p m328p -c usbasp -U optiboot_atmega328.hex
```
