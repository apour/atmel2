/* Measure temperature with DS18B20, set resolution to 0.1 C, return value in deci-degrees sent on UART */
#define F_CPU 10000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

// =======================
// UART
// =======================
#define BAUD 9600
#define MY_UBRR F_CPU/16/BAUD-1

void UART_Init(unsigned int ubrr) {
    // Set baud rate
    UBRRH = (unsigned char)(ubrr >> 8);
    UBRRL = (unsigned char)ubrr;
    // Enable receiver and transmitter
    UCSRB = (1 << RXEN) | (1 << TXEN);
    // Set frame format: 8 data bits, 1 stop bit
    UCSRC = (1 << URSEL) | (3 << UCSZ0);
}

void uart_send_char(char c) {
    while (!(UCSRA & (1 << UDRE)));
    UDR = c;
}

void uart_send_string(const char* s) {
    while (*s) uart_send_char(*s++);
}

// =======================
// ONE-WIRE (DS18B20)
// =======================
#define DQ_PORT PORTB
#define DQ_DDR  DDRB
#define DQ_PINR PINB
#define DQ_BIT  PB0

void ow_write_low() {
    DQ_DDR |= (1 << DQ_BIT);   // výstup low
    DQ_PORT &= ~(1 << DQ_BIT);
}

void ow_release() {
    DQ_DDR &= ~(1 << DQ_BIT);  // vstup = high (pull-up)
}

uint8_t ow_reset() {
    uint8_t i;

    ow_write_low();
    _delay_us(480);
    ow_release();
    _delay_us(70);
    i = (DQ_PINR & (1 << DQ_BIT)) == 0;
    _delay_us(410);
    return i;
}

void ow_write_bit(uint8_t bit) {
    ow_write_low();
    if (bit) {
        _delay_us(6);
        ow_release();
        _delay_us(64);
    } else {
        _delay_us(60);
        ow_release();
        _delay_us(10);
    }
}

uint8_t ow_read_bit() {
    uint8_t bit;
    ow_write_low();
    _delay_us(6);
    ow_release();
    _delay_us(9);
    bit = (DQ_PINR & (1 << DQ_BIT)) != 0;
    _delay_us(55);
    return bit;
}

void ow_write_byte(uint8_t byte) {
    for (uint8_t i = 0; i < 8; i++) {
        ow_write_bit(byte & 0x01);
        byte >>= 1;
    }
}

uint8_t ow_read_byte() {
    uint8_t byte = 0;
    for (uint8_t i = 0; i < 8; i++) {
        byte >>= 1;
        if (ow_read_bit())
            byte |= 0x80;
    }
    return byte;
}

// =======================
// DS18B20 čtení teploty
// =======================
int16_t ds18b20_read_temp() {
    uint8_t temp_l, temp_h;
    int16_t temp;

    ow_reset();
    ow_write_byte(0xCC);   // SKIP ROM
    ow_write_byte(0x44);   // CONVERT T

    // čekání na dokončení konverze
    while (!ow_read_bit());

    ow_reset();
    ow_write_byte(0xCC);   // SKIP ROM
    ow_write_byte(0xBE);   // READ SCRATCHPAD

    temp_l = ow_read_byte();
    temp_h = ow_read_byte();

    temp = (temp_h << 8) | temp_l;
 
    return (temp * 10) >> 4;    
}

// =======================
// MAIN
// =======================
int main(void) {
    char buffer[32];
    int16_t t = 25;
    UART_Init(MY_UBRR);

    uart_send_string("Start...\r\n");

    while (1) {
        t = ds18b20_read_temp();                
        uint8_t whole = t / 10;
        uint8_t dec   = t % 10;

        snprintf(buffer, sizeof(buffer), "Temp: %u.%u\r\n", whole, dec);
        uart_send_string(buffer);
        _delay_ms(1000);
    }
}