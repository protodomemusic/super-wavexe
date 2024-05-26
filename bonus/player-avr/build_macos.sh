#!/bin/bash

# compile the 4ML
python3 4ml-compiler.py roses-are-ff0000-backing-track.4ml

# build the program for upload
avr-gcc -Os -g -std=gnu99 -Wall -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums  -ffunction-sections -fdata-sections  -DF_CPU=8000000UL -DBAUD=9600UL -I. -I../../AVR-Programming-Library -mmcu=attiny85 -c -o output.o 4ml-avr-player.c
avr-gcc -Wl,-Map,output.map  -Wl,--gc-sections  -Wl,--relax -mmcu=attiny85 output.o  -o output.elf
avr-objcopy -j .text -j .data -O ihex output.elf output.hex

# flash to microcontroller
avrdude -c USBtiny -p attiny85 -B 1 -U flash:w:output.hex

# clean up
rm output.o
rm output.elf
rm output.hex
rm output.map