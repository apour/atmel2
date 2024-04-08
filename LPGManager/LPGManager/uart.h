/*****************************************************************************
* UART debug
*
*****************************************************************************/
//@{

#pragma once

// functions
extern void odDebugInit(void);
extern void uartPutc(char c);
extern void uartPutText(char *data, uint16_t len);
extern void uartPutTxt(char *data);
extern void uartWriteUInt8(uint8_t v);
extern void uartWriteUInt16(uint16_t v);
extern void uartWriteUInt32(uint8_t* v);