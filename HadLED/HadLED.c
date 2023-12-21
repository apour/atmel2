/*
 * HadLED.c
 *
 * Created: 30.1.2013 21:06:52
 *  Author: Ales
 */ 

#define F_CPU 12000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void had()
{
	PORTA = 0x01;   /* activate all pull-ups */
    DDRA = 0xff;    /* all pins output */ 
	PORTB = 0x01;   /* activate all pull-ups */
    DDRB = 0xff;    /* all pins output */ 
	PORTC = 0x01;   /* activate all pull-ups */
    DDRC = 0xff;    /* all pins output */ 
	PORTD = 0x01;   /* activate all pull-ups */
    DDRD = 0xff;    /* all pins output */ 
	int mask = 0x01;
    while(1)
    {
		_delay_ms(500);
		mask<<=1;
		if (mask==0x100) 
		{
			mask=1;
		}
		PORTA=~(mask&0xFF);			
		PORTB=~(mask&0xFF);
		PORTC=~(mask&0xFF);
		PORTD=~(mask&0xFF);
    }
}

int main(void)
{
	had();
}
