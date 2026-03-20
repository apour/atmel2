/*
 * ATmega8 + DS18B20 (1-Wire) — jednoduché čtení teploty a výpis přes UART.
 * MCU: ATmega8 @ F_CPU = 8 MHz (interní RC)
 *
 * Datový pin 1-Wire: PD6 (možno změnit níže)
 * UART: 9600 Bd, 8N1
 */

 /*
 MCU: ATmega8 @ 8 MHz (interní RC).
Datový pin 1‑Wire: PD6 (lze změnit v definicích níže).
Pull‑up rezistor: 4.7 kΩ mezi PD6 a VCC (nutné).
Napájení senzoru: 3.0–5.5 V (parazitní napájení je také možné, viz poznámka níže).
*/

#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>

// ----------- Nastavení pinů -----------
// 1-Wire na PD6
#define ONEWIRE_PORT  PORTD
#define ONEWIRE_PINR  PIND
#define ONEWIRE_DDR   DDRD
#define ONEWIRE_BIT   PD6

// UART: TX = PD1 (HW)
#define BAUD 9600
#define UBRR_VALUE ((F_CPU / (16UL * BAUD)) - 1)

// ----------- Pomocné makra pro pin 1-Wire -----------
static inline void ow_line_low(void) {
    ONEWIRE_DDR |= (1 << ONEWIRE_BIT);     // jako výstup
    ONEWIRE_PORT &= ~(1 << ONEWIRE_BIT);   // stáhnout na 0
}
static inline void ow_line_release(void) {
    ONEWIRE_DDR &= ~(1 << ONEWIRE_BIT);    // jako vstup (pull-up drží 1)
    // Nepovolujeme interní pull-up; spoléháme na externí 4.7k
}
static inline uint8_t ow_read_pin(void) {
    return (ONEWIRE_PINR & (1 << ONEWIRE_BIT)) ? 1 : 0;
}

// ----------- UART -----------
static void uart_init(void) {
    // Nastavení baud rate
    UBRRH = (uint8_t)(UBRR_VALUE >> 8);
    UBRRL = (uint8_t)(UBRR_VALUE & 0xFF);
    // Povolit TX
    UCSRB = (1 << TXEN);
    // 8N1
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
}

static void uart_putc(char c) {
    while (!(UCSRA & (1 << UDRE)));
    UDR = c;
}

static void uart_print(const char *s) {
    while (*s) uart_putc(*s++);
}

static void uart_print_int(int16_t v) {
    // jednoduchý tisk signed integeru
    if (v < 0) {
        uart_putc('-');
        v = -v;
    }
    char buf[7];
    uint8_t i = 0;
    do {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    } while (v && i < sizeof(buf));
    while (i--) uart_putc(buf[i]);
}

// ----------- 1-Wire základní operace -----------
/* Reset přítomnosti (presence) — vrací true, když senzor odpoví. */
static bool ow_reset(void) {
    bool presence = false;
    // master reset pulse: low min 480us
    ow_line_low();
    _delay_us(480);
    // uvolnit linii
    ow_line_release();
    _delay_us(70);
    // presence je nízká (0) od senzoru v okně 60–240us po uvolnění
    presence = (ow_read_pin() == 0);
    // počkej do konce slotu (celkem cca 480us)
    _delay_us(410);
    return presence;
}

static void ow_write_bit(uint8_t bit) {
    if (bit) {
        // write-1: low ~6us, pak release do konce slotu (~64us)
        ow_line_low();
        _delay_us(6);
        ow_line_release();
        _delay_us(64);
    } else {
        // write-0: low ~60us, pak release krátce
        ow_line_low();
        _delay_us(60);
        ow_line_release();
        _delay_us(10);
    }
}

static uint8_t ow_read_bit(void) {
    uint8_t r;
    // start read slot: master low ~6us
    ow_line_low();
    _delay_us(6);
    ow_line_release();
    // číst cca po ~9us od release
    _delay_us(9);
    r = ow_read_pin();
    // počkej na konec slotu (~55us)
    _delay_us(55);
    return r;
}

static void ow_write_byte(uint8_t b) {
    for (uint8_t i = 0; i < 8; i++) {
        ow_write_bit(b & 0x01);
        b >>= 1;
    }
}

static uint8_t ow_read_byte(void) {
    uint8_t b = 0;
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t bit = ow_read_bit();
        b >>= 1;
        if (bit) b |= 0x80;
    }
    return b;
}

// ----------- DS18B20 příkazy -----------
#define CMD_SKIP_ROM       0xCC
#define CMD_CONVERT_T      0x44
#define CMD_READ_SCRATCH   0xBE
#define CMD_WRITE_SCRATCH  0x4E
#define CMD_COPY_SCRATCH   0x48
#define CMD_READ_POWER     0xB4

// Volitelné: zjistit, zda je senzor v parazitním napájení
static bool ds18b20_is_parasite_powered(void) {
    if (!ow_reset()) return false; // pokud není přítomen, vrať false
    ow_write_byte(CMD_SKIP_ROM);
    ow_write_byte(CMD_READ_POWER);
    uint8_t pwr = ow_read_bit();
    // read_bit = 0 => parazitní napájení, 1 => napájený
    return (pwr == 0);
}

// Spustí konverzi teploty (blokující čekání do dokončení)
static bool ds18b20_convert_blocking(void) {
    if (!ow_reset()) return false;
    ow_write_byte(CMD_SKIP_ROM);
    ow_write_byte(CMD_CONVERT_T);

    // Pokud je napájený standardně, lze testovat „bus release“ jako hotovo.
    // Jednoduše ale počkáme maximální dobu pro 12bit rozlišení: ~750ms.
    // Můžeš zrychlit (nižší rozlišení) nebo polling read_bit().
    for (uint16_t i = 0; i < 750; i++) {
        _delay_ms(1);
    }
    return true;
}

// Přečte 9 bajtů scratchpadu do bufferu[9]
static bool ds18b20_read_scratchpad(uint8_t *buf9) {
    if (!ow_reset()) return false;
    ow_write_byte(CMD_SKIP_ROM);
    ow_write_byte(CMD_READ_SCRATCH);
    for (uint8_t i = 0; i < 9; i++) {
        buf9[i] = ow_read_byte();
    }
    return true;
}

// Jednoduchá 8-bit CRC Dallas (polynom x^8 + x^5 + x^4 + 1) – volitelně
static uint8_t dallas_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t inbyte = data[i];
        for (uint8_t j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}

// Převede hodnotu z Scratchpadu (Temp LSB/MSB) na desetiny stupně C (×10)
static bool ds18b20_read_temperature_x10(int16_t *temp_x10) {
    uint8_t sp[9];
    if (!ds18b20_convert_blocking()) return false;
    if (!ds18b20_read_scratchpad(sp)) return false;

    // Validuj CRC: CRC přes prvních 8 bajtů musí sedět s 9. bajtem
    uint8_t crc = dallas_crc8(sp, 8);
    if (crc != sp[8]) {
        return false; // CRC error
    }

    // Teplota je signed 16-bit, 1 LSB = 1/16 °C
    int16_t raw = (int16_t)((sp[1] << 8) | sp[0]); // MSB:sp[1], LSB:sp[0]
    // Převod na desetiny °C: (raw * 10) / 16
    // Opatrně s dělením pro zachování přesnosti
    int32_t t = (int32_t)raw * 10;
    t = t / 16;
    *temp_x10 = (int16_t)t;
    return true;
}

// ----------- main -----------
int main(void) {
    // Nastav 1-Wire pin jako vstup (uvolněno), externí pull-up drží 1
    ow_line_release();

    uart_init();
    _delay_ms(100);

    uart_print("DS18B20 na ATmega8 (8MHz), 1-Wire na PD6\r\n");

    bool parasite = ds18b20_is_parasite_powered();
    uart_print("Napajeni senzoru: ");
    uart_print(parasite ? "parazitni\r\n" : "klasicke\r\n");

    while (1) {
        int16_t t_x10;
        if (ds18b20_read_temperature_x10(&t_x10)) {
            // Výpis ve formátu X.Y °C
            int16_t whole = t_x10 / 10;
            int16_t frac  = t_x10 % 10;
            uart_print("Teplota: ");
            uart_print_int(whole);
            uart_putc('.');
            if (frac < 0) frac = -frac;
            uart_putc('0' + frac);
            uart_print(" C\r\n");
        } else {
            uart_print("Chyba cteni/CRC nebo nepritomny senzor.\r\n");
        }

        _delay_ms(1000);
    }
}
