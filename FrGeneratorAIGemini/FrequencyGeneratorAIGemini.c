#define F_CPU 10000000UL

#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>


volatile uint16_t last_capture = 0;
volatile uint16_t period_ticks = 0;

volatile uint8_t timer_overflows = 0;
volatile uint32_t final_time = 0;
volatile uint8_t edge_count = 0;
volatile uint8_t inMeasureCalculate = 0;
volatile uint8_t overlowMeasure = 0;

volatile uint8_t lastTimeHi = 0;
volatile uint8_t lastTimeLo = 0;

// Přerušení při přetečení 8-bitového časovače
ISR(TIMER0_OVF_vect) {
    if (inMeasureCalculate==1)
        return;
    timer_overflows++;
    if (timer_overflows == 0xFF) {
        overlowMeasure = 1;
        timer_overflows = 0;
    }
    //uart_send_char('O');
}

// Přerušení na INT0 (PD2) - měření vstupní frekvence
ISR(INT0_vect) {
    inMeasureCalculate = 1;
    if (edge_count == 0) {
        // První hrana - start měření
        
        TCNT0 = 0;
        timer_overflows = 0;
        edge_count = 1;
        overlowMeasure = 0;
        //uart_send_char('S');        
    } else {
        // Druhá hrana - konec měření
        uint8_t timer_val = TCNT0;        
                
        final_time = (timer_overflows << 8) + timer_val;
        final_time/= 2; // protože měříme periodu, ne frekvenci, vynásobíme 2    
        edge_count = 0; 
    
		OCR1AH = final_time >> 8; // vyšší byte
		OCR1AL = final_time & 0xFF; // nižší byte
		        
        lastTimeHi = timer_overflows;
        lastTimeLo = timer_val;

        timer_overflows = 0;
    }
    inMeasureCalculate = 0;
}

void main(void) {
    // 1. Nastavení pinu PB1 (OC1A) jako výstupní
    DDRB |= (1 << PB1);

    // 3. Konfigurace TCCR1A
    // COM1A0: Přepne (toggle) pin OC1A při každé shodě (Match)
    TCCR1A = (1 << COM1A0);

    // 4. Konfigurace TCCR1B
    // WGM12: Zapne režim CTC (Clear Timer on Compare Match)    
    // prescaler 64
    TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);

    // Nastavení INT0 na náběžnou hranu
    MCUCR |= (1 << ISC01) | (1 << ISC00);
    GIMSK |= (1 << INT0);

    // Nastavení Timer0: Prescaler 64 (příklad)    
    // CS01=1 | CS11=1 -> frekvence časovače = F_CPU / 64
    TCCR0 |= (1 << CS01) | (1 << CS00); 
    TIMSK |= (1 << TOIE0); // Povolení přerušení při přetečení Timer0
	
    timer_overflows = 0;
    uart_init();
	sei();
	
    while (1) {
        // Zde můžete měnit OCR1A pro změnu střídy
		_delay_ms(500);

        uart_send_char('T');    
        uart_send_char(':');
        //uart_send_hex((final_time >> 8) & 0xFF); // vyšší byte
        uart_send_hex(lastTimeHi);        // nižší byte
        uart_send_hex(lastTimeLo);        // nižší byte        
        if (overlowMeasure == 1) {
            uart_send_char(' '); // indikace přetečení
            uart_send_char('O'); // indikace přetečení
        }
        uart_send_char('\r');
        uart_send_char('\n');
        
    }
}
