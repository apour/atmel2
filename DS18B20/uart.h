#ifndef _UART_H
#define _UART_H
#include <avr/io.h>

void uart_init(); 
void uart_send_char(unsigned char ch); 
void uart_sendString(char *s); 
void uart_send_hex(unsigned char ch); 
void uart_send_dec(uint8_t value, uint8_t paddingZeros);

#endif 
