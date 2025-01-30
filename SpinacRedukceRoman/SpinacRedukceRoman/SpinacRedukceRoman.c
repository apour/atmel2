#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define F_CPU 16000000UL  // Define the clock frequency (8 MHz)

// LED connected to PB0
#define LED_PIN PB0

volatile unsigned int repeat_cnt0 = 0;
volatile unsigned int frequency = 0;

void interrupt0_init()
{
	// enable external interrupt 0
	DDRB &= ~_BV(2);
	PORTB |= _BV(2);
	MCUCR |= _BV(ISC01);
	GIMSK  |= _BV(INT0);
}

ISR (INT0_vect)
{	
	int n=5;
	n++;	
	frequency++;
}

// Function to initialize Timer0 to generate interrupts
void timer0_init()
{
	TCCR0A = 0x05;
	TCCR0B = 0x04;
	TCNT0 = 256 - 244;
	TIMSK |= _BV(TOIE0);   // enable interrupt for compare A
    sei();
}

ISR (TIMER0_OVF_vect)
{
	TCNT0 = 256 - 244;
    if (++repeat_cnt0 == 64)
	{
		repeat_cnt0 = 0;
		if (frequency > 10)
		{
			if (PORTB==1)
				PORTB = 0;
			else
				PORTB = 1;		
		}	
		frequency = 0;
	}	
}

int main()
{
    // Set PB0 (LED) as an output pin
    DDRB |= (1 << LED_PIN);
    
    // Initialize Timer0
    timer0_init();
	interrupt0_init();
	
    // Main loop
    while (1)
    {					
		_delay_ms(500);
        // Main loop does nothing, Timer0 interrupt handles the LED flashing
    }
}
