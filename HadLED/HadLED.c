/*
 * HadLED.c
 *
 * Created: 30.1.2013 21:06:52
 *  Author: Ales
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
void doPause()
{
	int i=100;
	int y=1000;
	while (i>0) 
	{
		i--;
		y=1000;
		while (y>0)
		{
			y--;
		}
	}
}

void doPauseQuick()
{
	int i=1;
	int y=1000;
	while (i>0) 
	{
		i--;
		y=1;
		while (y>0)
		{
			y--;
		}
	}
}

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
		doPause();
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

void testPin()
{
	PORTB = 0x20;   /* activate all pull-ups */
    DDRB = 0xff;    /* all pins output */ 
    while(1)
	{
		;
	}	
}

void ledFlashing()
{
	PORTB = 0x20;   /* activate all pull-ups */
    DDRB = 0xff;    /* all pins output */ 	
	int val = 0x01;
    while(1)
    {
		doPauseQuick();
		PORTB=~(val&0xFF);
		val = (val==0x01) ? 0x0 : 0x01;
    }
}


#include <avr/interrupt.h>
short timer0Cnt;

void timer0Init()
   {
   // clear timer0Cntr
   timer0Cnt=0;
   // enable interrupt from timer 0 
   TIMSK = TIMSK | _BV(TOIE0);
   // set CLOCK / 8
   TCCR0 = _BV(CS01);
   // set counter
   TCNT0 = 106;
   sei();
   }

int main(void)
{
	timer0Init();
	had();
	//testPin();
	ledFlashing();
}
