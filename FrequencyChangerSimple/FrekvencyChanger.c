/*
 * FrequencyChanger.c
 *
 * Created: 27.8.2022 18:13:48
 * Author: Ales
 *
 * MEASURE FREQUENCY ON PD3 (INT1) AND SEND INFO BY UART 19200 B
 * Input delta is set by voltage frequency on PIN PC1
 * Output Frequency is on PINT PC0
 * Output duty is on PC2
  *
 * Must be connected
 * -----------------
 * AGND - Ground
 * AREF - +5V
 * AVCC - +5V
 * 
 *
 */ 

//#define __AVR_ATmega8__
#define F_CPU 16000000UL // Clock Speed

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define ADJUST_VOLTAGE_FREQUENCY_INPUT_PIN				1
#define DUMP_TO_UART									1
#define TIMER0_CONST									256 - 200
#define TIMER2_CONST									256 - 183
#define SIGNAL_DETECTOR_MAX_COUNTER						6000 //1200

#define SIGNAL_DETECTOR_COUNTER_STOP_MEASURE_LIMIT		0x1400
#define SIGNAL_DETECTOR_COUNTER_START_MEASURE_LIMIT		0x1200

#define TCNT1_MIN_SIGNAL_LEN							81
#define TCNT1_SIGNALS_PER_SECONDS						15625
#define TCNT1_MIN_SIGNAL_LEN_DEF						TCNT1_SIGNALS_PER_SECONDS/2


// counters
volatile unsigned int repeat_cnt2=0;

// measure input signal
volatile unsigned int measureFrc=0;
volatile unsigned int signalDetectorCounter=0;
volatile unsigned int lastSignalDetectorCounter=0;
volatile unsigned int temp=0;

// output signal
volatile unsigned int outSignalLimit=0;
volatile unsigned int signalOutCounter=0;
volatile unsigned int delta=0;
volatile unsigned long deltaLong = 0;

volatile unsigned short needChange = 0;
volatile unsigned short needCheck = 0;
signed int voltage=0;				// input voltage for ADC - set Frequency diff

volatile unsigned short minimumFrequencyCounter = 0;
volatile unsigned int tempTCNT1 = 0;
volatile unsigned int lastUsedTCNT1 = 0;
volatile unsigned int TCNT1MinSignalLimit = 0;

enum
	{
	mWaitForSignal,
	mMeasure,
	mMeasureStopping
	}	
mMode;

//init adc
void init_ADC()   
	{ 
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS0);
	}
		
void timer0Init()
   {
   // enable interrupt from timer 0 
   TIMSK = TIMSK | _BV(TOIE0);
   // set CLOCK / 8
   TCCR0 = 0x02;//_BV(CS01) | _BV(CS00);
   // set counter
   TCNT0 = TIMER0_CONST;
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
   
void OutFrequenceChangeLogicLevel()
	{
	if (PORTC&_BV(PC0))
		{
		PORTC&= ~_BV(PC0);	
		}
	else
		{
		PORTC|= _BV(PC0);
		}	
	}			
      
SIGNAL(SIG_OVERFLOW0)
   {
   cli();
   // set counter
   TCNT0 = TIMER0_CONST;
   signalDetectorCounter++;
   if (signalDetectorCounter>=SIGNAL_DETECTOR_MAX_COUNTER)
		{
		// possible no signal
		signalDetectorCounter = 0;
		lastSignalDetectorCounter = 0;
		needChange = 0;
		needCheck = 0;
		mMode = mWaitForSignal;
		sei();
		return;
		}
   
	signalOutCounter++;
   
	if (signalOutCounter>outSignalLimit)
		{
		needChange = 1;
		signalOutCounter = 0;
		}    
   
   sei();  
   }


SIGNAL(INT1_vect)
	{
	cli();
	needCheck = 1;
	
	tempTCNT1 = TCNT1;
	lastUsedTCNT1 = tempTCNT1;
	TCNT1 = 0;
	
	minimumFrequencyCounter++;
	sei();
	}	

unsigned short isSignalStopLimit()
	{
	if (lastSignalDetectorCounter == 0)
		return 1;
	if (minimumFrequencyCounter < 1)
		return 1;
	return (lastSignalDetectorCounter > SIGNAL_DETECTOR_COUNTER_STOP_MEASURE_LIMIT) ? 1 : 0;
	}

unsigned short isSignalStartLimit()
	{
	if (lastSignalDetectorCounter == 0)
		return 0;
	if (minimumFrequencyCounter < 2)
		return 0;
	return (lastSignalDetectorCounter < SIGNAL_DETECTOR_COUNTER_START_MEASURE_LIMIT) ? 1 : 0;
	}

void checkSignalDetectionLimits()
	{
	switch (mMode)
		{
		case mMeasure:
			if (isSignalStopLimit())
			{
				mMode = mWaitForSignal;
#ifdef DUMP_TO_UART	  
	 			uartPutc('-');
#endif
			}
			break;
		case mWaitForSignal:
			if (isSignalStartLimit())
			{
				mMode = mMeasure;
#ifdef DUMP_TO_UART	  
	 			uartPutc('+');
#endif
			}
			break;
		case mMeasureStopping:
			mMode = mWaitForSignal;
#ifdef DUMP_TO_UART	  
	 			uartPutc('-');
#endif
			break;
		default:
			break;
		}
	}

ISR (TIMER2_OVF_vect)
	{
	TCNT2 = TIMER2_CONST;
	++repeat_cnt2;
	if (repeat_cnt2 == 64)	// 1s
		{
		repeat_cnt2 = 0;
		
		// read voltage for adjust frequency
		ADMUX = ADJUST_VOLTAGE_FREQUENCY_INPUT_PIN;
		ADCSRA |= (1<<ADSC); // Start conversion
		while (ADCSRA & (1<<ADSC)); // wait for conversion to complete
		voltage = ADCW; 

		checkSignalDetectionLimits();

#ifdef DUMP_TO_UART	  
		uartPutc(' ');
		uartPutc('M');
		uartPutc(':');
		uartWriteUInt8(mMode);	
		uartPutc(' ');
		uartPutc('F');
		uartPutc(':');
		uartWriteUInt8(minimumFrequencyCounter);	
        uartPutc(' ');
		uartPutc('P');
		uartPutc(':');
		uartWriteUInt16(lastUsedTCNT1);
		uartPutc(' ');
		uartPutc('V');
		uartPutc(':');
		uartWriteUInt16(voltage);	
		uartPutc(' ');
		uartPutc('C');
		uartPutc(':');
		uartWriteUInt16(delta);
		uartPutc(' ');
		uartPutc('S');
		uartPutc(':');
		uartWriteUInt16(lastSignalDetectorCounter);
		uartPutc(' ');
		uartPutc('\r');
		uartPutc('\n');
#endif				
		minimumFrequencyCounter	= 0;
		}
	}

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
	
	TCNT1=0x00;
	TCCR1B = (1<<CS12) | (1<<CS10);  // prescaler 1024
	
	GICR |= _BV(INT1);	/* enable INT1 */
	MCUCR|= _BV(ISC11);
	MCUCR|= _BV(ISC01);

	sei();
}
	
void adjustOutputSignal()
{
	delta = outSignalLimit;
	if (voltage>0x200)
	{
		deltaLong = (unsigned long) (voltage-0x200);
		deltaLong*= delta;
		deltaLong>>= 10;
		delta = (unsigned int) deltaLong;
		outSignalLimit+= delta;
	}
	else
	{
		deltaLong = (unsigned long) (0x200-voltage);
		deltaLong*= delta;
		deltaLong>>= 10;
		delta = (unsigned int) deltaLong;
		outSignalLimit-= delta;
	}
}

void checkForQuickDiff()
{
	if (minimumFrequencyCounter<2)
	{
		return;
	}
	// limit test -> big change means possible no signal
	unsigned int maxDiff = signalDetectorCounter;
	maxDiff=maxDiff/4;
	
	unsigned int fDiff = abs(lastSignalDetectorCounter - temp);
	if (fDiff > maxDiff)
	{
		mMode = mMeasureStopping;
	}
}

unsigned short isTCNT1InLimit()
{
	if (mMode == mMeasure)
	{
		//TCNT1MinSignalLimit = TCNT1_MIN_SIGNAL_LEN_DEF / 4 / minimumFrequencyCounter;
		TCNT1MinSignalLimit = TCNT1_MIN_SIGNAL_LEN_DEF / 2 / minimumFrequencyCounter;
		if (lastUsedTCNT1 < TCNT1MinSignalLimit)
			return 0;	
	}
	
	return 1;
}	


int main(void)  
{
	hardwareInit();

#ifdef DUMP_TO_UART	  		
	odDebugInit();
	uartPutTxt("START");
#endif	

	timer0Init();
	timer2Init();
	init_ADC();
	
	mMode = mWaitForSignal;

    sei();
	
	while (1)
		{
		if (needChange && mMode == mMeasure)
			{
			OutFrequenceChangeLogicLevel();
			needChange = 0;
			}
		
		if (needCheck)
			{
			needCheck = 0;
			temp = lastSignalDetectorCounter;
			lastSignalDetectorCounter = signalDetectorCounter;		
			
			//checkForQuickDiff();
			if (isTCNT1InLimit())
			{
				// valid signal - recalculate
				outSignalLimit = signalDetectorCounter/2;
				adjustOutputSignal();					
	
				signalDetectorCounter=0;		
				if (signalOutCounter>outSignalLimit)
					{
					signalOutCounter = 0;
					needChange = 1;
					}
				}		
			}
			
		}			
		
	return 1;
}	