/*
 * _78hc595.c
 *
 * Created: 20.12.2022 11:19:49
 *  Author: Ales
 */ 

#define CONTROL_PORT 	 PORTB
#define Data_Pin		  0
#define Enable_Pin		  4
#define Latch_Clk_Pin     2     
#define Shift_Clk_Pin     1

/*---------------------------------------------------------------------
-------------------CONTROL BITS OF SHIFT REGISTER ---------------------
This is basically just renaming everything to make it easy to work with
-----------------------------------------------------------------------*/
#define Data_H						CONTROL_PORT|=_BV(Data_Pin)
#define Data_L						CONTROL_PORT&=~_BV(Data_Pin)
#define Clear_Enable  				CONTROL_PORT|=_BV(Enable_Pin)
#define Set_Enable	 				CONTROL_PORT&=~_BV(Enable_Pin)
#define Shift_Clk_H			   		CONTROL_PORT|=_BV(Shift_Clk_Pin)
#define Shift_Clk_L			    	CONTROL_PORT&=~_BV(Shift_Clk_Pin)
#define Latch_Clk_H			    	CONTROL_PORT|=_BV(Latch_Clk_Pin)
#define Latch_Clk_L				 	CONTROL_PORT&=~_BV(Latch_Clk_Pin)
#define delay(a)					_delay_ms(a)

#include <avr/io.h>
#include <avr/delay.h>

uint8_t mask;
uint16_t mask16;

void writeByte(uint8_t data)
{
	mask = 0x80;
	Latch_Clk_L;
	delay(10);
	while (mask>0)
	{
		Shift_Clk_L;
		if (data&mask)
			Data_H;	
		else
			Data_L;
		delay(20);   // these delays are arbitrary - they are here to slow things down so you can see the pattern
		Shift_Clk_H;			
		delay(20);		
		
		mask>>=1;
	}
	Latch_Clk_H;
}

void writeUInt16(uint16_t data)
{
	mask16 = 0x8000;
	Latch_Clk_L;
	delay(10);
	while (mask16>0)
	{
		Shift_Clk_L;
		if (data&mask16)
			Data_H;	
		else
			Data_L;
		delay(20);   // these delays are arbitrary - they are here to slow things down so you can see the pattern
		Shift_Clk_H;			
		delay(20);		
		
		mask16>>=1;
	}
	Latch_Clk_H;
}


int main(void)
{
	DDRB = 0xFF;
	uint16_t d = 0x101;
	//writeByte(0x05);
	writeUInt16(0x7777);
	while(1)
    {
    }
}