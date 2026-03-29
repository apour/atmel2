// uart.c

#include "uart.h"
#include <avr/io.h>

#define F_CPU 10000000UL

#ifdef __AVR_ATmega8__
// for AT MEGA 8
#define  ODDBG_UBRR  UBRRL
#define  ODDBG_UCR   UCSRB
#define  ODDBG_TXEN  TXEN
#define  ODDBG_USR   UCSRA
#define  ODDBG_UDRE  UDRE
#define  ODDBG_UDR   UDR
#endif

#define UART_SPEED 9600

void uart_init() {
    ODDBG_UCR |= (1<<ODDBG_TXEN);
	ODDBG_UBRR = F_CPU / (UART_SPEED * 16L) - 1;
}


void uart_send_char(unsigned char ch)
{
   UDR = ch;

   /* Wait for empty transmit buffer */
   while (! (UCSRA & _BV(UDRE)) );

}

void uart_sendString(char *s)
{
   unsigned int i=0;
   while (s[i] != '\x0') 
   {
       uart_send_char(s[i++]);
    };
}

void uart_send_hex(unsigned char ch)
{
    unsigned char i,temp;     
    for (i=0; i<2; i++)
    {
        temp = (ch & 0xF0)>>4;
        if ( temp <= 9)
            uart_send_char ( '0' + temp);
        else
            uart_send_char(  'A' + temp - 10);
        ch = ch << 4;    
     }   
}


void uart_send_dec(uint8_t value, uint8_t paddingZeros) {
    unsigned char hundreds = value / 100;
    if (hundreds > 0 || paddingZeros == 1)
        uart_send_char('0' + hundreds);
    value %= 100;
    unsigned char tens = value / 10;
    if (tens > 0 || hundreds > 0 || paddingZeros == 1)
        uart_send_char('0' + tens);
    unsigned char ones = value % 10;    
    uart_send_char('0' + ones);
}


