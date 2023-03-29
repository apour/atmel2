/*
 * FrequencyChanger.c
 *
 * Created: 13.3.2023 18:13:48
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
#include <avr/delay.h>

#define ADJUST_VOLTAGE_FREQUENCY_INPUT_PIN				1
#define DUMP_TO_UART									1
#define TIMER0_CONST									256 - 100
#define TIMER2_CONST									256 - 183
#define SIGNAL_DETECTOR_MAX_COUNTER						6000 //1200

#define TIMER0_CYCLES_PER_SECOND						6000
#define TCNT1_MIN_SIGNAL_LEN							81

#define TCNT1_SIGNALS_PER_SECONDS						15625
#define TCNT1_MIN_SIGNAL_LEN_DEF						TCNT1_SIGNALS_PER_SECONDS/2


// counters
volatile unsigned int repeat_cnt0=0;

// measure input signal
volatile unsigned int temp=0;

// output signal
volatile unsigned long deltaLong = 0;

volatile unsigned short signalDetected = 0;
signed int voltage=0;				// input voltage for ADC - set Frequency diff

volatile unsigned int frequencyCounter = 0;
volatile unsigned int lastFrequencyCounter = 0;
volatile unsigned int tempFrequencyCounter = 0;
volatile unsigned int calcDeltaLastValid = 0;
volatile unsigned int outCounter = 0;
volatile unsigned int delta = 0;
volatile unsigned int calcDelta = 0;

volatile unsigned short needRecalc = 0;
volatile unsigned short needRefresh = 0;
	
volatile unsigned int tempTCNT1 = 0;
volatile unsigned int filteredFrequencyCounter = 0;

volatile unsigned int lastUsedTCNT1 = 0;
volatile unsigned int TCNT1MinSignalLimit = 0;

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
      
ISR(TIMER0_OVF_vect)
	{
	TCNT0 = TIMER0_CONST;
	
	// code every 0.05 ms
	repeat_cnt0++;
	
	if (repeat_cnt0 == TIMER0_CYCLES_PER_SECOND/2)
	{
		// read voltage for adjust frequency
		ADMUX = ADJUST_VOLTAGE_FREQUENCY_INPUT_PIN;
		ADCSRA |= (1<<ADSC); // Start conversion
		while (ADCSRA & (1<<ADSC)); // wait for conversion to complete
		voltage = ADCW; 	
	}
			
	if (repeat_cnt0 == TIMER0_CYCLES_PER_SECOND)
		{
		// 1 s
		repeat_cnt0 = 0;
#ifdef DUMP_TO_UART	  
		uartPutc('F');
		uartWriteUInt16(frequencyCounter);
		uartPutc(' ');
		uartPutc('O');
		uartWriteUInt16(filteredFrequencyCounter);
		uartPutc(' ');
		uartPutc('P');
		uartWriteUInt16(lastUsedTCNT1);
		uartPutc(' ');
		uartPutc('L');
		uartWriteUInt16(TCNT1MinSignalLimit);
		uartPutc(' ');
#endif						

		//lastFrequencyCounter = frequencyCounter;
		tempFrequencyCounter = frequencyCounter;
		lastFrequencyCounter = filteredFrequencyCounter;
		frequencyCounter = 0;
		filteredFrequencyCounter = 0;
		needRecalc = 1;
		signalDetected = 1;

		TCNT1MinSignalLimit = TCNT1_MIN_SIGNAL_LEN;
		if (signalDetected == 1)
		{
			if (lastFrequencyCounter > 0)
			{
				//TCNT1MinSignalLimit = TCNT1_MIN_SIGNAL_LEN_DEF / 2 / tempFrequencyCounter;
				TCNT1MinSignalLimit = TCNT1_MIN_SIGNAL_LEN_DEF / 4 / lastFrequencyCounter;
			}	
		}
				
		//if (lastFrequencyCounter < 3)
		if (lastFrequencyCounter < 2)
			{
			signalDetected = 0;
			}
		
#ifdef DUMP_TO_UART
		uartPutc('C');
		uartWriteUInt16(TCNT1MinSignalLimit);
		uartPutc(' ');	  
		uartPutc('D');
		uartWriteUInt16(calcDelta);
		uartPutc(' ');
		uartPutc('V');
		uartWriteUInt16(voltage);
		uartPutc(' ');
		if (signalDetected)
		{
			uartPutc('+');
		}
		else
		{
			uartPutc('-');
		}
		uartPutc('\r');
		uartPutc('\n');
#endif						
		}
		
	outCounter++;
	if (outCounter == delta)
		{
		if (signalDetected)
			{
			OutFrequenceChangeLogicLevel();	
			}
		outCounter = 0;
		if (needRefresh == 1)
			{
			delta = calcDelta;
			needRefresh = 0;
			}
		}
	}

SIGNAL(SIG_INTERRUPT1)
	{
	cli();	
	frequencyCounter++;
	tempTCNT1 = TCNT1;
	lastUsedTCNT1 = tempTCNT1;
	TCNT1 = 0;
	
	//if (tempTCNT1 > TCNT1_MIN_SIGNAL_LEN) // minimum signal length
	if (tempTCNT1 > TCNT1MinSignalLimit)
		{
		filteredFrequencyCounter++;
		}

	sei();
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
	
int main(void)  
{
	hardwareInit();

#ifdef DUMP_TO_UART	  		
	odDebugInit();
	uartPutTxt("START");
#endif	

	timer0Init();
	init_ADC();
	
    sei();
	
	while (1)
		{
		if (needRecalc == 1)
			{
			if (lastFrequencyCounter > 0)
				{			
				calcDelta = TIMER0_CYCLES_PER_SECOND/lastFrequencyCounter;
				calcDelta/= 2;
				needRecalc = 0;
			
				temp = calcDelta;
				if (voltage>0x200)
					{
					deltaLong = (unsigned long) (voltage-0x200);
					deltaLong*= temp;
					deltaLong>>= 10;
					temp = (unsigned int) deltaLong;
					calcDelta+= temp;
					}
				else
					{
					deltaLong = (unsigned long) (0x200-voltage);
					deltaLong*= temp;
					deltaLong>>= 10;
					temp = (unsigned int) deltaLong;
					calcDelta-= temp;
					}
				needRefresh = 1;
			
#ifdef DUMP_TO_UART	  
				uartPutc('T');
				uartWriteUInt16(temp);
#endif			
				}					
				
			}				
		}			
	return 1;
}	