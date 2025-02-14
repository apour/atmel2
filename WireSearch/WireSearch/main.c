// 
// Anpassungen im makefile:
//    ATMega8 => MCU=atmega8 im makefile einstellen
//    lcd-routines.c in SRC = ... Zeile anhängen
// 
#include <avr/io.h>
#include <avr/interrupt.h>
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
		
		lcd_setcursor( 0, 0 );
		convertNumber(temp);
		// erneut Text ausgeben, aber diesmal komfortabler als String
		lcd_string("Freq:");
		lcd_string(displayBuffer);
	}
		
}

int main(void)
{
  // Initialisierung des LCD
  // Nach der Initialisierung müssen auf dem LCD vorhandene schwarze Balken
  // verschwunden sein
  lcd_init();
  enableInt0();
  timer0Init();

  // Text in einzelnen Zeichen ausgeben
  lcd_data( 'T' );
  lcd_data( 'e' );
  lcd_data( 's' );
  //lcd_data( 't' );

  // Die Ausgabemarke in die 2te Zeile setzen
  lcd_setcursor( 0, 2 );

  // erneut Text ausgeben, aber diesmal komfortabler als String
  lcd_string("Ahojda AP!");

  while(1)
  {
  }

  return 0;
}