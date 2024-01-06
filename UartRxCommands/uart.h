#ifndef _UART_H
#define _UART_H

#include <avr/io.h>

void uart_init();
void uartPutTxt(char *data);
void uartWriteUInt8(uint8_t v);
void uartWriteUInt16(uint16_t v);
void uartWriteUInt32(uint8_t* v);
void uart_flush();
//void sendstring();


#endif 
