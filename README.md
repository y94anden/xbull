# xBull

Code for Arduino Nano to use as slave for RS485 network

Written for avr-gcc (no Arduino libraries used).


# Problems

* When two devices share the same address, the reply from one must not
  be read as a request to the other. One way would be to have a different
  command when replying, ie 0x01=read, 0x02=read reply. 0x81=write,
  0x82=write reply

# Pins

These pins are used by the system. See `hardware.c` for more info.

```
                         -|TXD     VIN|-
                         -|RXD     GND|-
                         -|RESET RESET|-
                         -|GND      5V|-
         RS485 direction -|PD2    ADC7|-
                         -|PD3    ADC6|-
                         -|PD4     PC5|- WS1812b led chain
                         -|PD5     PC4|-
                         -|PD6     PC3|-
                         -|PD7     PC2|-
                         -|PB0     PC1|- button / random source
                         -|PB1     PC0|- onewire
                SPI /SS  -|PB2    AREF|-
                SPI MOSI -|PB3     3V3|-
                SPI MISO -|PB4     PB5|- LED / SPI SCK
                               USB
```

# Bestyckning

## Datorrum
Plintar på:
* PC0 - onewire
* PC1
* PC4 - DHT11
* PC5
* ADC7
* 5V

## Pannrum
Plintar på:
* PB0
* PB1
* PC0 - onewire
* PC1
* PC2
* PC3
* ADC7
* 5V
Kopplat onewire till sensorerna runt pannrummet och i datorrummet.
