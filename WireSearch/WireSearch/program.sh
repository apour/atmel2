PROJECTNAME=$1
avrdude -p m8 -c usbasp -U flash:w:$1.hex