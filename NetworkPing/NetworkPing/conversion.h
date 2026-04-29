/*
 * Conversion.h
 *
 * Created: 19.2.2017 20:57:52
 *  Author: Ales
 */ 

#ifndef CONVERSION_H_
#define CONVERSION_H_

// functions
extern unsigned short convertUInt16ToText(char* text, unsigned long v, short padding);
extern unsigned short convertUInt32ToText(char* text, unsigned long v, short padding);
extern unsigned short convertUInt8ToText(char* text, unsigned long v, short padding);
extern unsigned short convertUInt8ToHexText(char* text, char v);

#endif /* CONVERSION_H_ */