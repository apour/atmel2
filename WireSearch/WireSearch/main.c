// 
// Anpassungen im makefile:
//    ATMega8 => MCU=atmega8 im makefile einstellen
//    lcd-routines.c in SRC = ... Zeile anhängen
// 
// Show frequency in PD2 INT0 on lcd display on port B
// Beep piezo on PC6

#ifndef F_CPU
#define F_CPU 16000000
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "lcd.h"

#define TIMER_CONST	256 - 244
#define PIEZO_MASK	0x20
#define FREQUENCY_LO_LIMIT	30000
unsigned int repeated_cnt0 = 0;

#define BUFFER_SIZE	6
char displayBuffer[BUFFER_SIZE];
unsigned int dividers[] = {10000, 1000, 100, 10, 1, 0};
unsigned int curFrequencyLo = 0;
unsigned int curFrequencyHi = 0;
unsigned int frequencyLo = 0;
unsigned int frequencyHi = 0;
unsigned short needShowData=0;
unsigned short foundWire = 0;
unsigned short idx;

typedef struct
{
	unsigned int loMin;
	unsigned int loMax;
} T_Limit_Item_Lo;

#define LIMITS_HI_COUNT	3
#define LIMITS_LO_COUNT 14

T_Limit_Item_Lo limits_lo[] = { {1720, 1790}, {16450, 16500}, {8210, 8250}, {19110, 19125}, {9552, 9566}, {4775, 4783}, 
								{2386, 2394}, {1190, 1198}, {1321, 1328}, {659, 665}, {330, 334}, {162, 168}, {81, 85}, 
								{39, 43} };
unsigned short limits_hi[] = { 5, 2, 1 };
	
void convertNumber(unsigned int number, unsigned int* dividersBegin)
{
	number++;
	unsigned int* divider = dividersBegin;
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
	curFrequencyLo++;
	if (curFrequencyLo==FREQUENCY_LO_LIMIT)
	{
		curFrequencyHi++;
		curFrequencyLo=0;
	}
	sei();
	}
	
void timer0Init()
{
	TCCR0 = 0x03; // divider 64
	TCNT0 = TIMER_CONST;
	TIMSK |= _BV(TOIE0);
	sei();		
}

unsigned short findWireIdxByHiLimits()
{
	unsigned short i = 0;
	while (i<LIMITS_HI_COUNT)
	{
		if (limits_hi[i] == frequencyHi)
			return i+1;
		i++;
	}
	return 0;
}

unsigned short findWireIdxByLoLimits(unsigned short from, unsigned short to)
{
	unsigned short i = from;
	while (i<to)
	{
		if (limits_lo[i].loMin < frequencyLo && limits_lo[i].loMax > frequencyLo)
		    return i+1;
		i++;
	}
	return 0;
}

unsigned short findWireIdx()
{
	idx = findWireIdxByHiLimits();
	if (idx>0 && idx<LIMITS_HI_COUNT+1)
	{
		idx = findWireIdxByLoLimits(0, LIMITS_HI_COUNT+1);
		if (idx>0 && idx<LIMITS_HI_COUNT+1)
		{
			return idx;
		}
		return 0;
	}
	return findWireIdxByLoLimits(LIMITS_HI_COUNT, LIMITS_LO_COUNT);
}

ISR (TIMER0_OVF_vect)
{
	TCNT0 = TIMER_CONST;
	if (foundWire>0)
	{
		if ((PORTC&PIEZO_MASK)==PIEZO_MASK)
		{
			PORTC&=~PIEZO_MASK;
		}
		else
		{
			PORTC|=PIEZO_MASK;
		}
	}
	
	if (++repeated_cnt0 == 1024)
	//if (++repeated_cnt0 == 1)
	{
		cli();
		repeated_cnt0 = 0;
		frequencyLo = curFrequencyLo;
		frequencyHi = curFrequencyHi;
		curFrequencyLo = 0;
		curFrequencyHi = 0;
		sei();

		convertNumber(frequencyHi, &dividers[3]);
		lcd_setcursor( 7, 1 );
		lcd_string(displayBuffer);
		lcd_data(':');
		
		convertNumber(frequencyLo, &dividers[0]);
		// erneut Text ausgeben, aber diesmal komfortabler als String
		lcd_setcursor( 10, 1 );
		lcd_string(displayBuffer);
		
		foundWire = findWireIdx();
		unsigned int temp = foundWire;
		convertNumber(temp, &dividers[3]);
		lcd_setcursor( 7, 2 );
		
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
	lcd_string(" Wire finder");
	lcd_setcursor( 1, 2 );
	lcd_string("by Ales Pour");
	_delay_ms(2000);

	lcd_clear();
	lcd_setcursor( 1, 1 );
	lcd_string("Freq: ");

    lcd_setcursor( 1, 2 );
	lcd_string("Wire: ");
	
	DDRC=0x20;
	enableInt0();
	timer0Init();

	while (1)	
	{
	}

	return 0;
}