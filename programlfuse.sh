LFUSE=$1
if [ -z "$1" ]; then
    echo "Use programlfuse.sh '\xXX' for program lFuse."
else
    printf "%b" "$LFUSE" > /tmp/lfuse.bin
    avrdude -p m8 -c usbasp -U lfuse:w:/tmp/lfuse.bin
fi

