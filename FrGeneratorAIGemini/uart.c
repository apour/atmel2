// uart.c

#include "uart.h"
#include <avr/io.h>

#define F_CPU 10000000UL
#define UART_SPEED	9600

#ifdef __AVR_ATmega8__
// for AT MEGA 8
#define  ODDBG_UBRR  UBRRL
#define  ODDBG_UCR   UCSRB
#define  ODDBG_TXEN  TXEN
#define  ODDBG_USR   UCSRA
#define  ODDBG_UDRE  UDRE
#define  ODDBG_UDR   UDR
#endif

void uart_init() 
{
    // fosc = 8000000 Hz 
    // baud=9600, actual_baud=9615,4, err=0,2 %
    // UBRRL = 0x33; 
    // UBRRH = 0x00;
	
	// fosc = 12000000 Hz 
    // baud=9600, actual_baud=9615,4, err=0,2 %
    //UBRRL = 0x4D; 
    //UBRRH = 0x00; 
   //
    //// enable uart N81  
    //UCSRB =  _BV(RXEN) | _BV(TXEN) ;
    //UCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);
   //
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

void  uart_send_hex(unsigned char ch)
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

