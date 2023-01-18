/*********************************************
 * UART Debug
 *********************************************/
#define F_CPU   16000000L    /* evaluation board runs on 12MHz */

#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include "uart.h"

//#define UART_SPEED	57600
#define UART_SPEED	19200

#ifdef __AVR_ATmega8__
// for AT MEGA 8
#define  ODDBG_UBRR  UBRRL
#define  ODDBG_UCR   UCSRB
#define  ODDBG_TXEN  TXEN
#define  ODDBG_USR   UCSRA
#define  ODDBG_UDRE  UDRE
#define  ODDBG_UDR   UDR
#endif

#ifdef __AVR_ATmega328P__
// for AT MEGA 328
#define  ODDBG_UBRR  UBRR0L
#define  ODDBG_UCR   UCSR0B
#define  ODDBG_TXEN  TXEN0
#define  ODDBG_USR   UCSR0A
#define  ODDBG_UDRE  UDRE0
#define  ODDBG_UDR   UDR0
#endif

void odDebugInit(void)
{
    ODDBG_UCR |= (1<<ODDBG_TXEN);
	ODDBG_UBRR = F_CPU / (UART_SPEED * 16L) - 1;
}

void uartPutc(char c)
{
    while(!(ODDBG_USR & (1 << ODDBG_UDRE)));    /* wait for data register empty */
    ODDBG_UDR = c;
	
	while(!(ODDBG_USR & (1 << ODDBG_UDRE)));    /* wait for data register empty */
}

void uartPutText(char *data, uint16_t len)
{
    while(len--){
        uartPutc(*data++);
    }
} 

void uartPutTxt(char *data)
{
	uint8_t len = strlen( (const char*) data);
	if (len>32)
	{
		uartPutText("Problem", 7);
		return;
	}		
	uartPutText(data, len);
}

void uartOutDigit(uint8_t v)
{
	if (v<10)
		uartPutc(v+0x30);
	else 
		uartPutc(v+0x41-10);
}

void uartWriteUInt8(uint8_t v)
{
	uint8_t temp = v>>4;
	uartOutDigit(temp);
	temp = v&0xF;
	uartOutDigit(temp);
}	

void uartWriteUInt16(uint16_t v)
{
	uint8_t hi = v>>8;
	uartWriteUInt8(hi);
	hi = v&0xFF;
	uartWriteUInt8(hi);
}	

void uartWriteUInt32(uint8_t* v)
{
	v+=3;
	uint8_t hi = *v;
	uartWriteUInt8(hi);
	v--;
	
	hi = *v;
	uartWriteUInt8(hi);
	v--;
	
	hi = *v;
	uartWriteUInt8(hi);
	v--;
	
	hi = *v;
	uartWriteUInt8(hi);
	//uartWriteUInt16((uint16_t) v);
	/*uint8_t hi = v>>24;
	uartWriteUInt8(hi);
	hi = v>>16;
	uartWriteUInt8(hi);
	hi = v>>8;
	uartWriteUInt8(hi);
	hi = v&0xFF;
	uartWriteUInt8(hi);*/
}	
