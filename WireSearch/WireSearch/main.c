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
#include <avr/delay.h>
#include "lcd.h"

#define TIMER_CONST	256 - 244
unsigned int repeated_cnt0 = 0;

#define BUFFER_SIZE	6
char displayBuffer[BUFFER_SIZE];
unsigned int dividers[] = {10000, 1000, 100, 10, 1, 0};
unsigned int frequency = 0;

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
	DDRB &= ~_BV(2);
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
	if (++repeated_cnt0 == 4)
	{
		repeated_cnt0 = 0;
		unsigned int temp = frequency;
		frequency = 0;
				
		convertNumber(temp);
		// erneut Text ausgeben, aber diesmal komfortabler als String
		lcd_setcursor( 1, 1 );
		lcd_string("Freq: ");
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
  lcd_setcursor( 0, 0 );
  lcd_string("Freq: ");

  _delay_us(1000);  
  
  enableInt0();
  timer0Init();
//
  while(1)
  {
  }

  return 0;
}