/*
 * StudnaSpinac.c
 *
 * Created: 9/14/2024 11:18:26 AM
 *  Author: admin
 */ 

#include <avr/io.h>
#include "uart.h"

/*
		Water level
		-----------
		
		-
		-  Start PINC 1
		-
		- 
		-  Stop PINC 2
		-
		-  + for PINC
		-


*/
#define SETBIT(ADDRESS,BIT) (ADDRESS |= (1<<BIT))
#define CLEARBIT(ADDRESS,BIT) (ADDRESS &= ~(1<<BIT))

typedef enum {
    WaitForStart, Running
} StateMachineMode;

unsigned char relayState = 0;
StateMachineMode state_mode;

void uart_endofline()
{
    uart_sendString("\n\r");
}

void switchOffRelay()
{
    if (relayState == 0)
    {
        return;
    }
    relayState = 0;
    CLEARBIT(PORTB, 0);
    uart_sendString("SWITCH OFF RELAY");
    uart_endofline();
}

void switchOnRelay()
{
    if (relayState == 1)
    {
        return;
    }
    relayState = 1;
    SETBIT(PORTB, 0);
    uart_sendString("SWITCH ON RELAY");
    uart_endofline();
}

void changeMode(StateMachineMode mode)
{
    CLEARBIT(PORTB, 2);
    CLEARBIT(PORTB, 3);
	CLEARBIT(PORTB, 4);
	switch (mode)
	{
		case WaitForStart:
			SETBIT(PORTB, 4);		// green led
			switchOffRelay();
			state_mode = WaitForStart;
			break;
		case Running:
			SETBIT(PORTB, 2);		// red led
			switchOnRelay();
			state_mode = Running;
			break;
	}			
}

int main(void)
{
	DDRC = 0x0;
	PINC = 0x0;
	
	DDRB = 0xFF;
	PORTB = 0x0;
	DDRD = 0xFF;
	PORTD = 0x00;
	uint8_t ledState = 0;
	state_mode = WaitForStart;
	//while(1)
    //{
		//uint8_t data = PINC;	
		////PORTB = data;
		//PORTD = data;
		////ledState++;
		////PORTD = ledState;
		//_delay_ms(1000);
	//}
			
    while(1)
    {
        switch (state_mode)
		{
			case WaitForStart:
				if ((PINC&0x03) == 0x03)
					changeMode(Running);
				break;
			case Running:
				if ((PINC&0x03) == 0x0)
					changeMode(WaitForStart);
				break;
		}
		_delay_ms(1000);
		if (ledState==0)
		{
			SETBIT(PORTB, 3);
			ledState = 1;
		}
		else
		{
			CLEARBIT(PORTB, 3);
			ledState = 0;
		}										
    }
}