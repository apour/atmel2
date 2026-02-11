#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>

// UART 9600 8N1
void UART_init(void)
{
    uint16_t ubrr = 103;   // 16MHz / (16*9600) - 1

    UBRRH = (unsigned char)(ubrr >> 8);
    UBRRL = (unsigned char)ubrr;

    UCSRB = (1 << TXEN);   // povolit TX

    UCSRC = (1 << URSEL) |     // DŮLEŽITÉ u ATmega8
            (1 << UCSZ1) | (1 << UCSZ0);  // 8 bitů
}

void UART_send(char data)
{
    while (!(UCSRA & (1 << UDRE)));
    UDR = data;
}

// INT0 na náběžnou hranu
void INT0_init(void)
{
    DDRD &= ~(1 << PD2);   // PD2 jako vstup
    PORTD |= (1 << PD2);   // pull-up

    // Nastavit náběžnou hranu
    MCUCR |= (1 << ISC01) | (1 << ISC00);

    GICR |= (1 << INT0);   // povolit INT0
}

ISR(INT0_vect)
{
    UART_send('A');
}

int main(void)
{
    UART_init();
    INT0_init();
    sei();   // globální povolení přerušení

    while (1)
    {
    }
}
