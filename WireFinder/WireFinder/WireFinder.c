/*
 * WireFinder.c
 *
 * Created: 2/5/2025 6:22:41 PM
 *  Author: admin
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

#define TIMER_CONST	256-100
#define WIRES_COUNT	16

volatile unsigned short wire[WIRES_COUNT];
unsigned short curWireValue;
unsigned short mask;

void init()
{
	for (unsigned short i=0; i<WIRES_COUNT; i++)
	{
		wire[i] = i+1;
	}	
	// set PORTC as output
	// set PORTD as output
	DDRC = 0xFF;
	DDRD = 0xFF;
}

void timer0Init()
{
	TCCR0 = 0x02; // divider 8
	TCNT0 = TIMER_CONST;
	TIMSK |= _BV(TOIE0);
	sei();		
}

void toggleBit(unsigned short bit)
{
	mask = 1;
	if (bit<8)
	{
		mask<<= bit;
		PORTD^= mask;
		return;
	}
	bit-=8;
	mask<<= bit;
	PORTC^= mask;
}

ISR (TIMER0_OVF_vect)
{
	TCNT0 = TIMER_CONST;
	for (unsigned short i=0; i<WIRES_COUNT; i++)
	{
		curWireValue = wire[i]-1;
		if (curWireValue>0)
		{
			wire[i]=curWireValue;
			continue;
		}
		wire[i] = i+1;
		toggleBit(i);
	}	
}

int main(void)
{
	init();
	timer0Init();
    while(1)
    {
        ;
    }
}