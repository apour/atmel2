// 
// Anpassungen im makefile:
//    ATMega8 => MCU=atmega8 im makefile einstellen
//    lcd-routines.c in SRC = ... Zeile anhängen
// 
// Show frequency in PD2 INT0 on lcd display on port C

#ifndef F_CPU
#define F_CPU 16000000
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "lcd.h"

#define TIMER_CONST	256 - 244
unsigned int repeated_cnt0 = 0;

#define BUFFER_SIZE	6
char displayBuffer[BUFFER_SIZE];
unsigned int dividers[] = {10000, 1000, 100, 10, 1, 0};
unsigned int frequency = 0;
unsigned int counter = 0x20;
unsigned short needShowData=0;

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

void enableInt0()
{
	// enable external interrupt 0
	MCUCR |= _BV(ISC01);
	MCUCR |= _BV(ISC00);
	GIMSK  |= _BV(INT0);
	sei();
}

SIGNAL(INT0_vect)
	{
	cli();
	frequency++;
	sei();
	}
	
void timer0Init()
{
	TCCR0 = 0x05; // divider 1024
	TCNT0 = TIMER_CONST;
	TIMSK |= _BV(TOIE0);
	sei();		
}

ISR (TIMER0_OVF_vect)
{
	TCNT0 = TIMER_CONST;
	if (++repeated_cnt0 == 64)
	{
		repeated_cnt0 = 0;
		unsigned int temp = frequency;
		//unsigned int temp = counter;
		frequency = 0;
		//counter++;

		convertNumber(temp);

		// erneut Text ausgeben, aber diesmal komfortabler als String
		lcd_setcursor( 7, 1 );

		lcd_string(displayBuffer);
	}
		
}

int main(void)
{
	// Initialisierung des LCD
	// Nach der Initialisierung müssen auf dem LCD vorhandene schwarze Balken
	// verschwunden sein
	lcd_init();

	lcd_clear();
	// Die Ausgabemarke in die 2te Zeile setzen
	lcd_setcursor( 1, 1 );
	lcd_string("AP Test");
	_delay_ms(2000);

	lcd_clear();
	lcd_setcursor( 1, 1 );
	lcd_string("Freq: ");
	counter = 0;

	enableInt0();
	timer0Init();

	while (1)	
	{
	}

	return 0;
}