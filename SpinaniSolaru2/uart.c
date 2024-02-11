#define F_CPU 12000000UL

#include "uart.h"
#include <string.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define BUF_LEN 64
#define USART_BAUDRATE 1200
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

char out[BUF_LEN];
volatile unsigned char tx_send;
volatile unsigned int outp;
volatile unsigned int inp;

void uart_init()
	{
    UCSRB |= (1 << TXEN) | (1 << RXEN)| (1 << TXCIE) | (1 << RXCIE); // enable tx, rx
    UBRRH = (BAUD_PRESCALE >> 8); // set baud
    UBRRL = BAUD_PRESCALE;
    UCSRC=(1<<URSEL)|(0<<UMSEL)|(0<<UPM1)|(0<<UPM0)|
        (0<<USBS)|(0<<UCSZ2)|(1<<UCSZ1)|(1<<UCSZ0); 
    tx_send = 0;
    inp = 0;
	}

void sendCurChar()
	{
	if (out[outp] != '\0')
		UDR = out[outp];
	UCSRB |= (1<<UDRIE);
	}

void sendstring() 
	{  
  	outp = 0;
  	tx_send = 1;
  	sendCurChar();
	}

ISR(USART_UDRE_vect) 
	{
	UCSRB &= ~(1<<UDRIE);
   	if (out[outp] == '\0') 
		{
    	tx_send = 0;
  		} 
	else 
		{
    	outp++;
		sendCurChar();
  		}
	}

// use only following public functions
void uart_flush()
	{
   	out[inp] = '\0';
   	sendstring();
   	inp=0;  
	}

void uartPutTxt(char *data)
	{
	uint8_t len = strlen( (const char*) data);
	if (inp+len>BUF_LEN)
		{
    	out[inp] = '!'; inp++;
    	uart_flush();
		return;
		}		
  	while (*data != '\0')
  	{
    	out[inp] = *data;
     	data++;
     	inp++;
  	}
	}

void uartOutDigit(uint8_t v)
	{
	if (v<10)
		out[inp]=(v+0x30);
	else 
		out[inp]=(v+0x41-10);
    inp++;
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
	}	


