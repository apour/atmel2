/*
 * WireFinder.c
 *
 * Created: 2/5/2025 6:22:41 PM
 *  Author: admin
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

#define TIMER0_CONST	256-20
#define TIMER1_CONST	65536-6000

unsigned short curWireValuePortD;
unsigned short curWireValuePortC;
unsigned short mask;

void init()
{
	DDRC = 0xFF;
	DDRD = 0xFF;
	PORTD = 0;
	PORTC = 0;
	curWireValuePortD=0;
	curWireValuePortC=0;
}

void timer0Init()
{
	TCCR0 = 0x01; // divider 8
	TCNT0 = TIMER0_CONST;
	TIMSK |= _BV(TOIE0);
	sei();		
}

ISR (TIMER0_OVF_vect)
{
	TCNT0 = TIMER0_CONST;
	curWireValuePortD++;
	if (curWireValuePortD==0xFF)
	{
		curWireValuePortD = 0;
	}
	PORTD = curWireValuePortD;	
}

void timer1Init()
{
	// Set Timer1 to normal mode
	TCCR1A = 0x00;  // Normal mode, no PWM or CTC
	TCCR1B = 0x01;  // Prescaler of 8 (CS11 = 1, CS10 = 0, CS12 = 0)

	// Optional: Enable overflow interrupt (if you want to use interrupts)
	TIMSK |= (1 << TOIE1);  // Enable Timer1 overflow interrupt

	// Set the initial value of Timer1 (optional)
	TCNT1 = TIMER1_CONST;  // Start counting from 0
	sei();
}

// Timer1 overflow interrupt service routine
ISR(TIMER1_OVF_vect) 
{	
	TCNT1 = TIMER1_CONST;  // Start counting from 0
	curWireValuePortC++;
	if (curWireValuePortC == 0x80)
	{
		curWireValuePortC = 0;
	}	
	PORTC = curWireValuePortC;	
}

int main(void)
{
	init();
	timer0Init();
	timer1Init();
    while(1)
    {
        ;
    }
}
