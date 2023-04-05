avr-gcc -g -Os -mmcu=atmega8 -c quicktest.c
avr-gcc -g -mmcu=atmega8 -o quicktest.elf quicktest.o
avr-objdump -h -S quicktest.elf > quicktest.lst
avr-objcopy -j .text -j .data -O ihex quicktest.elf quicktest.hex
