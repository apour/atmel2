HFUSE=$1
if [ -z "$1" ]; then
    echo "Use programhfuse.sh '\xXX' for program hFuse."
else
    printf "%b" "$HFUSE" > /tmp/hfuse.bin
    avrdude -p m8 -c usbasp -U hfuse:w:/tmp/hfuse.bin
fi

