/*
 * WireFinder.c
 *
 * Created: 2/5/2025 6:22:41 PM
 *  Author: admin
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

#define TIMER_CONST	256-20
#define WIRES_COUNT	16

#define BUFFER_SIZE	6
char displayBuffer[BUFFER_SIZE];
unsigned int dividers[] = {10000, 1000, 100, 10, 1, 0};

volatile unsigned short wire[WIRES_COUNT];
unsigned int curWireValue;
unsigned short mask;

void convertNumber(unsigned int number)
{
	number++;
	unsigned int* divider = dividers;
	unsigned short v = 0;
	char* p = displayBuffer;
	unsigned int curDivider = 0;
	while (*divider > 0)
	{
		curDivider = *divider;
		v = 0;
		while (number>curDivider)
		{
			number-= curDivider;
			v++;
		}
		v+= 0x30;
		*p = v;
		divider++;
		p++;
	}	
	*p = '\0';
}

void init()
{
	DDRC = 0xFF;
	DDRD = 0xFF;
	curWireValue=0;
}

void timer0Init()
{
	TCCR0 = 0x01; // divider 8
	TCNT0 = TIMER_CONST;
	TIMSK |= _BV(TOIE0);
	sei();		
}

ISR (TIMER0_OVF_vect)
{
	TCNT0 = TIMER_CONST;
	curWireValue++;
	PORTC = curWireValue&0xFF;
	mask = curWireValue>>8;
	PORTD = mask;	
}

int main(void)
{
	convertNumber(59999);
	init();
	timer0Init();
    while(1)
    {
        ;
    }
}