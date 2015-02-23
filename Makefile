# Arduino mini Makefile - see https://github.com/sudar/Arduino-Makefile/
ARD_TAG=ethernet
ARDUINO_PORT=/dev/ttyACM0
ARDUINO_LIBS=Solight Ethernet Ethernet/utility SPI

include /usr/share/arduino/Arduino.mk
