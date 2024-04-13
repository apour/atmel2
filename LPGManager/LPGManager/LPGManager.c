/*
 * LPGManager.c
 *
 * Created: 4/4/2024 6:19:57 PM
 *  Author: Ales
 
 * Connection Display: CLK - PINB5, DIN - PINB3, DC - PINB0, RST - PINB1, CE - PINB2
 * Start impuls PIND7
 * Relay PINC1-PINC6
 */ 

#define DUMP_TO_UART
#define TIMER2_CONST									256 - 183

//#define __AVR_ATmega8__
#define F_CPU 12000000UL // Clock Speed

#define MOTOR_START_FRC_LO_LIMIT	80

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "n3310.h"

// counters
volatile unsigned int repeat_cnt2=0;
// measure input signal
volatile unsigned int measureFrc=0;
volatile unsigned int lastMeasureFrc=0;

// running time
unsigned short runningTimeS=0;
unsigned short runningTimeM=0;
unsigned short runningTimeH=0;

char buffer[15];
unsigned int temp;
unsigned short mask;
unsigned short tempUInt8;

enum
	{
	mWaitForStart,
	mStarting,
	mRunning,
	mStopping
	}	
mMode;

void checkMachineState()
{
	switch (mMode)
	{
		case mWaitForStart:
		    tempUInt8 = PIND;
			mask = _BV(PD7);
			tempUInt8=tempUInt8&mask;
			if (tempUInt8 == mask)
			{
				mMode = mStarting;
			}
			break;
		case mStarting:
			if (lastMeasureFrc>MOTOR_START_FRC_LO_LIMIT)
			{
				mMode = mRunning;
			}
			break;
		case mRunning:
			if (lastMeasureFrc<MOTOR_START_FRC_LO_LIMIT)
			{
				mMode = mStopping;
			}
			break;
		default:
			break;
	}	
}

void increseRunningTime()
{
	runningTimeS++;
	if (runningTimeS==60)
	{
		runningTimeS=0;
		runningTimeM++;
	}
	if (runningTimeM==60)
	{
		runningTimeM=0;
		runningTimeH++;
	}
}

void resetRunningTime()
{
	runningTimeS=runningTimeM=runningTimeH=0;
}

static void hardwareInit(void)
{
    //PORTD = 0xfa;   /* 1111 1010 bin: activate pull-ups except on USB lines */
    PORTD = 0x0;
	DDRD = 0x02;    /* 0000 0010 bin: remove USB reset condition */
	
    // set PC as output
	PORTC = 0;
	DDRC = _BV(PC1) | _BV(PC2) | _BV(PC3) | _BV(PC4) | _BV(PC5);
	
	// SET INT1 for input frequency
	PORTD|= _BV(PD3);
		
	GICR |= _BV(INT1);	/* enable INT1 */
	MCUCR|= _BV(ISC11);
	MCUCR|= _BV(ISC01);

	resetRunningTime();
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
	if (repeat_cnt2 == 32 || repeat_cnt2 == 64)
		{
		checkMachineState();
		}		
	if (repeat_cnt2 == 64)	// 1s
		{
		repeat_cnt2 = 0;
		temp = measureFrc;
		lastMeasureFrc = measureFrc;
		measureFrc = 0;
		
//#ifdef DUMP_TO_UART	  
		//uartPutc(' ');
		//uartPutc('F');
		//uartPutc(':');
		//uartWriteUInt16(measureFrc);	
		//uartPutc(' ');
//#endif
		//measureFrc = 0;
		if (mMode==mRunning)
		{
			increseRunningTime();
		}
		
		sprintf(buffer, "Time:%03d:%02d:%02d", runningTimeH, runningTimeM, runningTimeS);
		LCD_gotoXY(0,1);
		LCD_writeString_F(buffer);
		uartPutTxt(buffer);
		uartPutTxt("\r\n");
		
		sprintf(buffer, "Mode: %d", mMode);
		LCD_gotoXY(0,3);
		LCD_writeString_F(buffer);
		//uartPutTxt(buffer);
		//uartPutTxt("\r\n");
		
		sprintf(buffer, "Frc: %d  ", temp);
		LCD_gotoXY(0,2);
		LCD_writeString_F(buffer);
		uartPutTxt(buffer);
		uartPutTxt("\r\n");
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