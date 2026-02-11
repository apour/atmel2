#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint16_t last_capture = 0;
volatile uint16_t period = 0;

void INT0_init(void)
{
    DDRD &= ~(1 << PD2);      // PD2 vstup
    PORTD |= (1 << PD2);      // pull-up

    MCUCR |= (1 << ISC01) | (1 << ISC00);  // náběžná hrana
    GICR |= (1 << INT0);                   // povolit INT0
}

void Timer1_init(void)
{
    TCCR1B |= (1 << CS11);   // prescaler 8 (0.5 µs tick)
}

void Timer2_init(void)
{
    DDRD |= (1 << PD7);  // PD7 jako výstup

    TCCR2 |= (1 << WGM21);  // CTC mód
    TCCR2 |= (1 << COM20);  // toggle OC2 při compare
    TCCR2 |= (1 << CS21);   // prescaler 8
}

ISR(INT0_vect)
{
    uint16_t now = TCNT1;
    period = now - last_capture;
    last_capture = now;

    if (period > 0)
    {
        // Zvýšení frekvence o 23 %
        uint16_t new_period = period / 1.23;

        // Timer2 je 8bit → přepočet
        // toggle režim → perioda = 2*(OCR2+1)*tick
        uint16_t ocr = (new_period / 2) - 1;

        if (ocr < 255)
            OCR2 = (uint8_t)ocr;
    }
}

int main(void)
{
    Timer1_init();
    Timer2_init();
    INT0_init();

    sei();

    while (1)
    {
    }
}
