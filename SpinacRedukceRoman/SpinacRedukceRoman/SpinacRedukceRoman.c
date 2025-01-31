#define F_CPU 16000000UL  // Define the clock frequency (8 MHz)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// LED connected to PB0
#define LED_PIN			PB0
#define RELAY_PIN		PB1
#define TCNT0_FOR_1S	256-244

volatile unsigned int repeat_cnt0 = 0;
volatile unsigned int frequency = 0;
volatile unsigned int frequencyTemp = 0;
typedef enum { t_relayOn, t_relayOff } TRelayMode;
TRelayMode relayMode;

void interrupt0_init()
{
	// enable external interrupt 0
	DDRB &= ~_BV(2);
	MCUCR |= _BV(ISC01);
	MCUCR |= _BV(ISC00);
	GIMSK  |= _BV(INT0);
}

ISR (INT0_vect)
{	
	frequency++;
}

void switchOnRelay()
{
	if (relayMode == t_relayOn) return;
	PORTB|= (1<<RELAY_PIN);
	relayMode = t_relayOn;
}

void switchOffRelay()
{
	if (relayMode == t_relayOff) return;
	PORTB&=~(1<<RELAY_PIN);
	relayMode = t_relayOff;
}

// Function to initialize Timer0 to generate interrupts
void timer0_init()
{
	TCCR0B = 0x04;			// clock / 256
	TCNT0 = TCNT0_FOR_1S;
	TIMSK |= _BV(TOIE0);   // enable interrupt for compare A
}

ISR (TIMER0_OVF_vect)
{
	TCNT0 = TCNT0_FOR_1S;
    if (++repeat_cnt0 == 256)
	{
		repeat_cnt0 = 0;
		frequencyTemp = frequency;
		frequency = 0;
		if (frequencyTemp > 40)
			switchOffRelay();
		else
			switchOnRelay();
	}	
}

int main()
{
    // Set PB0 (LED) as an output pin
	DDRB = 0;
    DDRB |= (1 << LED_PIN) | (1 << RELAY_PIN);
    PORTB = 0;
	relayMode = t_relayOff;
	switchOnRelay();
	
    // Initialize Timer0
    timer0_init();
	interrupt0_init();
	sei();
	
    // Main loop
    while (1)
    {					
    }
}
