/*
 * LPGManager.c
 *
 * Created: 4/4/2024 6:19:57 PM
 *  Author: Ales
 
 * Connection: CLK - PINB5, DIN - PINB3, DC - PINB0, RST - PINB1, CE - PINB2
 */ 
#define DUMP_TO_UART
#define TIMER2_CONST									256 - 183

//#define __AVR_ATmega8__
#define F_CPU 12000000UL // Clock Speed

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "n3310.h"

// counters
volatile unsigned int repeat_cnt2=0;
// measure input signal
volatile unsigned int measureFrc=0;

volatile unsigned int timerCounter=0;
char buffer[15];

enum
	{
	mWaitForStart,
	mStarting,
	mRunning
	}	
mMode;

static void hardwareInit(void)
{
    PORTD = 0xfa;   /* 1111 1010 bin: activate pull-ups except on USB lines */
    DDRD = 0x02;    /* 0000 0010 bin: remove USB reset condition */
	
    // set PC0 and PC2 as output
	DDRC = _BV(PC0) | _BV(PC2);
	PORTC = 0;
	
	// SET INT1 for input frequency
	PORTD|= _BV(PD3);
	
	// set PC0 and PC2 as output
	DDRC = _BV(PC0) | _BV(PC2);
	PORTC = 0;
	
	GICR |= _BV(INT1);	/* enable INT1 */
	MCUCR|= _BV(ISC11);
	MCUCR|= _BV(ISC01);

	sei();
}


void timer2Init()
   {
   // enable interrupt from timer 0 
   TIMSK = TIMSK | _BV(TOIE2);
   // set CLOCK / 1024
   TCCR2 = 0x07;
   // set counter
   TCNT2 = TIMER2_CONST;
   sei();
   }
   
SIGNAL(INT1_vect)
{
	cli();
	measureFrc++;
	sei();
}
	
ISR (TIMER2_OVF_vect)
	{
	TCNT2 = TIMER2_CONST;
	++repeat_cnt2;
	if (repeat_cnt2 == 64)	// 1s
		{
		repeat_cnt2 = 0;
		
//#ifdef DUMP_TO_UART	  
		//uartPutc(' ');
		//uartPutc('F');
		//uartPutc(':');
		//uartWriteUInt16(measureFrc);	
		//uartPutc(' ');
//#endif
		//measureFrc = 0;

		timerCounter++;
		sprintf(buffer, "Cnt: %d", timerCounter);
		LCD_gotoXY(0,1);
		LCD_writeString_F(buffer);
		uartPutc('A');
		uartPutTxt(buffer);
		uartPutTxt("\r\n");
		
		sprintf(buffer, "Mode: %d", mMode);
		LCD_gotoXY(0,2);
		LCD_writeString_F(buffer);
		//uartPutTxt(buffer);
		//uartPutTxt("\r\n");
		
		//LCD_writeString_F("LCD Nokia Test");
		}
	}				
		
int main(void)
{
	hardwareInit();

#ifdef DUMP_TO_UART	  		
	odDebugInit();
	uartPutTxt("START");
#endif	

	timer2Init();
	
	mMode = mWaitForStart;

    sei();
	
    DDRB = 0xFF;	// port pro pripojení displeje
	spi_init();		// inicializace sbernice
	LCD_init();		// inicializace displeje
		
	LCD_clear();
	LCD_gotoXY(0,0);
	LCD_writeString_F("LCD Nokia Test");
    
	while(1)
    {
		;    
    }

}