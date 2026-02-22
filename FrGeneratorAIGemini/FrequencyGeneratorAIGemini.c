#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>

volatile uint16_t last_capture = 0;
volatile uint16_t period_ticks = 0;

volatile uint32_t timer_overflows = 0;
volatile uint32_t final_time = 0;
volatile uint8_t edge_count = 0;

// Přerušení při přetečení 8-bitového časovače
ISR(TIMER0_OVF_vect) {
    timer_overflows++;
}

// Přerušení na INT0 (PD2) - měření vstupní frekvence
ISR(INT0_vect) {
    if (edge_count == 0) {
        // První hrana - start měření
        TCNT0 = 0;
        timer_overflows = 0;
        edge_count = 1;
    } else {
        // Druhá hrana - konec měření
        uint8_t timer_val = TCNT0;
        // Celkový počet tiků = (počet přetečení * 256) + aktuální hodnota
        final_time = (timer_overflows << 8) + timer_val;
        edge_count = 0; 
    
		OCR1AH = timer_overflows;
		OCR1AL = timer_val;
		timer_overflows = 0;    
        // Zde můžete s final_time dále pracovat (např. poslat po UARTu)
    }
}

void main(void) {
    // 1. Nastavení pinu PB1 (OC1A) jako výstupní
    DDRB |= (1 << PB1);

    // 2. Nastavení OCR1A registru (horní mez pro porovnání)
    // Příklad: Pro 1kHz při 8MHz hodinách a preskaleru 8:
    // OCR1A = (8 000 000 / (2 * 8 * 1000)) - 1 = 499
    OCR1A = 1499;

    // 3. Konfigurace TCCR1A
    // COM1A0: Přepne (toggle) pin OC1A při každé shodě (Match)
    TCCR1A = (1 << COM1A0);

    // 4. Konfigurace TCCR1B
    // WGM12: Zapne režim CTC (Clear Timer on Compare Match)
    // CS11: Nastaví preskaler na 8
    TCCR1B = (1 << WGM12) | (1 << CS11);

    // Nastavení INT0 na náběžnou hranu
    MCUCR |= (1 << ISC01) | (1 << ISC00);
    GIMSK |= (1 << INT0);

    // Nastavení Timer0: Prescaler 8 (příklad)
    // CS01=1 -> frekvence časovače = F_CPU / 8
    TCCR0 |= (1 << CS01); 
    TIMSK |= (1 << TOIE0); // Povolení přerušení při přetečení Timer0
	
	sei();
	
    while (1) {
        // Zde můžete měnit OCR1A pro změnu střídy
		_delay_ms(10);
    }
}
