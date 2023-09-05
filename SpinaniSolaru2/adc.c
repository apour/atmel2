// adc.c

#include "prj.h"

void adc_init() 
{ 
    // divider = 128  
	// fadc =  f = 93,7500 kHz  
    ADCSRA |= 0xE7; 
} 
 
unsigned int adc_get_value(unsigned char ch) 
{ 
    ADMUX = ch; 
    ADCSRA |= 0xE7; 
	_delay_ms(10); 
	return ( (unsigned int ) ( ADCL + (ADCH<<8) ) ); 
} 

