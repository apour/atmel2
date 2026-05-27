#include <avr/io.h>
#include <util/delay.h>

#define F_CPU 16000000UL

// Piny
#define SER PB0
#define SCK PB1
#define RCK PB2
#define RESET PB3

// Mapování segmentů (abcdefg + DP)
// Upravit podle typu displeje (common cathode/anode)
uint8_t digits[10] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111  // 9
};

// Odeslání 1 bytu do 74LS595
void shiftOut(uint8_t data) {
    uint8_t mask = 0x80;
    while (mask > 0)
    {
        if (data & mask)
            PORTB |= (1 << SER);
        else
            PORTB &= ~(1 << SER);
        mask >>= 1;
          
        // clock pulse
        PORTB |= (1 << SCK);
        _delay_ms(1); // krátká prodleva pro stabilitu
        PORTB &= ~(1 << SCK);
    }
}

// Resetování 74LS595
void reset(void) {
    PORTB &= ~(1 << RESET);
    _delay_ms(1000); // krátká prodleva pro stabilitu
    PORTB |= (1 << RESET);
}

// Překlopení registru (latch)
void latch(void) {
    PORTB |= (1 << RCK);
    _delay_ms(1000); // krátká prodleva pro stabilitu
    PORTB &= ~(1 << RCK);
}

int main(void) {
    // Nastavení pinů jako výstup
    DDRB |= (1 << SER) | (1 << SCK) | (1 << RCK) | (1 << RESET);
    reset(); // Reset 74LS595
    latch();
    PORTB |= (1 << SER);
    PORTB |= (1 << SCK);

     while (1) {
     
        for (uint8_t i = 0; i < 10; i++) {
            shiftOut(~digits[i]);
            latch();
            _delay_ms(5000);
        }
    }
}