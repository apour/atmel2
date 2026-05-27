PROJECTNAME=$1
rm $1.hex
rm $1.elf
echo "Compile" $1
avr-gcc -g -Os -mmcu=atmega8  -Wl,-Map,$PROJECTNAME.map -o $PROJECTNAME.elf  $PROJECTNAME.c
echo "Create hex files "
avr-objcopy -j .text -j .data -O ihex $PROJECTNAME.elf $PROJECTNAME.hex
echo "Dump files"
avr-objdump -h -S $1.elf > $1.lst