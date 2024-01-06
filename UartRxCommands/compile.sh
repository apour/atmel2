avr-gcc -g -Os -mmcu=atmega8 -c rxcommands.c uart.c
avr-gcc -g -mmcu=atmega8 -o rxcommands.elf rxcommands.o uart.o
avr-objdump -h -S rxcommands.elf > rxcommands.lst
avr-objcopy -j .text -j .data -O ihex rxcommands.elf rxcommands.hex
