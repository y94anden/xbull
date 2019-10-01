# xBull

Code for Arduino Nano to use as slave for RS485 network

Written for avr-gcc (no Arduino libraries used).


# Problems

* When two devices share the same address, the reply from one must not
  be read as a request to the other. One way would be to have a different
  command when replying, ie 0x01=read, 0x02=read reply. 0x81=write,
  0x82=write reply
