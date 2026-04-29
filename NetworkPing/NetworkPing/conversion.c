/*
 * Conversion.c
 *
 * Created: 19.2.2017 20:56:56
 *  Author: Ales
 */ 
static char temp;

unsigned short convertNumberToText(char* text, unsigned long v, unsigned long divisor, short padding)
{
	char* startTextPos = text;
	unsigned long temp = v;
	char c;
	short wasNonZero = 0;
	do 
	{
		c = 0x30;
		while (temp>=divisor)
		{
			temp-= divisor;
			c++;
			wasNonZero = 1;
		}
		divisor/= 10;
		// solve padding with zeros
		if (padding==0)
		{
			if (wasNonZero)
			{
				*text = c; text++;
			}
		}
		else
		{
			*text = c; text++;
		}
	} while (divisor>0);
	return text - startTextPos;
}

unsigned short convertUInt32ToText(char* text, unsigned long v, short padding)
{
	return convertNumberToText(text, v, 1000000000, padding);
}

unsigned short convertUInt16ToText(char* text, unsigned long v, short padding)
{
	return convertNumberToText(text, v, 100000, padding);
}

unsigned short convertUInt8ToText(char* text, unsigned long v, short padding)
{
	return convertNumberToText(text, v, 1000, padding);
}

unsigned short convertUInt8ToHexText(char* text, char v)
{
	temp = v>>4;
	if (temp<10)	
		*text = 0x31+temp-1;
	else
		*text = 0x41+temp-10;
	text++;
	temp = v&0xF;
	if (temp<10)	
		*text = 0x31+temp-1;
	else
		*text = 0x41+temp-10;
	text++;
	return 2;
}