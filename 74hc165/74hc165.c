/*
 * _74hc165.c
 *
 * Created: 12.5.2024
   Author: Ales
   Testing program for convert parallel input to serial with swift register chip
   Connect digital parallel input to PORTB, Output is on PORTC
 */ 

#define CONTROL_PORT_OUT 	PORTB
#define CONTROL_PORT_IN 	PINB
#define Data_Pin			0
#define LD_Pin				3   
#define Shift_Clk_Pin		1

/*---------------------------------------------------------------------
-------------------CONTROL BITS OF SHIFT REGISTER ---------------------
This is basically just renaming everything to make it easy to work with
-----------------------------------------------------------------------*/
#define Data_H						CONTROL_PORT_OUT|=_BV(Data_Pin)
#define Data_L						CONTROL_PORT_OUT&=~_BV(Data_Pin)
#define Shift_Clk_H			   		CONTROL_PORT_OUT|=_BV(Shift_Clk_Pin)
#define Shift_Clk_L			    	CONTROL_PORT_OUT&=~_BV(Shift_Clk_Pin)
#define LD_Clk_H			    	CONTROL_PORT_OUT|=_BV(LD_Pin)
#define LD_Clk_L				 	CONTROL_PORT_OUT&=~_BV(LD_Pin)
#define delay(a)					_delay_ms(a)

#include <avr/io.h>
#include <avr/delay.h>

uint16_t readData;
uint8_t temp;

uint16_t readUInt16()
{
	DDRB|= _BV(Shift_Clk_Pin) | _BV(LD_Pin);
	DDRB&= ~_BV(Data_Pin); // Data_Pin for read
	
	uint16_t readData=0;
	// Raise LD pin
	LD_Clk_H;
	for (uint16_t mask=1; mask!=0; mask<<=1)
	{
		if (CONTROL_PORT_IN&_BV(Data_Pin))
		{
			readData|=mask;
		}
				
		// create serial clock
		Shift_Clk_L;		
		delay(1);
		Shift_Clk_H;		
	}
	// Lower LD pin
	LD_Clk_L;		
	return readData;	
}

uint8_t readUInt8()
{
	DDRB|= _BV(Shift_Clk_Pin) | _BV(LD_Pin);
	DDRB&= ~_BV(Data_Pin); // Data_Pin for read
	
	uint8_t readData=0;
	// Raise LD pin
	LD_Clk_H;
	for (uint8_t mask=1; mask!=0; mask<<=1)
	{
		if (CONTROL_PORT_IN&_BV(Data_Pin))
		{
			readData|=mask;
		}
				
		// create serial clock
		Shift_Clk_L;		
		delay(1);
		Shift_Clk_H;		
	}
	// Lower LD pin
	LD_Clk_L;		
	return readData;	
}


int main(void)
{
	DDRC = 0xFF;
	while(1)
    {
		readData = readUInt16();
		//temp = (uint8_t) (readData>>8);
		temp = (uint8_t) (readData&0xFF);
		PORTC = temp;
		delay(100);	
    }
}