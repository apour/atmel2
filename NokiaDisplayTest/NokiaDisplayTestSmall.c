/*
 * NokiaDisplayTestSmall.c
 *
 * Created: 21.10.2019 18:47:30
 *  Author: Ales
 * Connected Nokia 3310 display to port B
 * Connection: CLK - PINB5, DIN - PINB3, DC - PINB0, RST - PINB1, CE - PINB2
 */ 

#include <avr/io.h>

int main(void)
{
    DDRB = 0xFF;	// port pro pripojeni displeje
	spi_init();		// inicializace sbernice
	LCD_init();		// inicializace displeje
		
	LCD_clear();
	LCD_writeString_F("AP Test");
    
	while(1)
    {
		;    
    }

}