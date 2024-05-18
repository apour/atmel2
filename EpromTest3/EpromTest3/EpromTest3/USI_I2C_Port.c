// url http://www.elektromys.eu/clanky/avr_twi/clanek.html
// A - P?enos 1 byte Master >> Slave (kód pro master)
#include <avr/io.h>
#define F_CPU 12000000
#include <util/delay.h>
#include <util/twi.h>

#define PORT_LED	PORTD
#define DDR_LED   	DDRD
#define P_LED_1		PD0			// used for init & data signal (for Debug)
#define P_LED_2		PD1			// used for debug
#define P_LED_3		PD2			// used for errors

#define SLV1_ADDRESS 0xA0                      // adresy r?zných slave za?ízení na sb?rnici
	
#define TW_SEND_START TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)
#define TW_SEND_STOP TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO)
#define TW_SEND_DATA TWCR = (1<<TWINT) | (1<<TWEN)
#define TW_WAIT while(!(TWCR & (1<<TWINT)))

char i2c_posli_1B(char adresa, char data, char data2, char data3);
char res=0;

int main(void){
 /* Initialize LED blinkers  */
 DDR_LED = _BV(P_LED_1) | _BV(P_LED_2) | _BV(P_LED_3);		      // enable output
	
 TWSR|= (1<<TWPS0) | (1<<TWPS1);
 TWBR=32;                                  // žádná p?edd?li?ka, F_SCL = 8MHz/(16+2*TWBR) = 100kHz.
 TWCR = (1<<TWEN);                         // zapnout TWI modul	
 DDRB &=~((1<<DDB0) | (1<<DDB1));          // dv? tla?ítka
 PORTB = (1<<PORTB0) | (1<<PORTB1);        // pull-up pro tla?ítka
 while(1){
    res = i2c_posli_1B(SLV1_ADDRESS,0,1,0x55);            //pošli data=1 pro slave 1
    if (res == 0)
	{
		blinkEm(1, P_LED_1);
	}
	else
	{
		blinkEm(1, P_LED_2);
		blinkEm(res, P_LED_3);
		
	}
	_delay_ms(1000);
 }
}

char i2c_posli_1B(char adresa, char data, char data2, char data3){                                   
 TW_SEND_START;                             // vygenerovat START sekvenci
	
 TW_WAIT;                                   // po?kat na odezvu TWI
 if ((TW_STATUS) != TW_START){ return 1;}   // pokud nebyl start vygenerován, máme error a kon?íme
 TWDR = adresa;                             // nahrát adresu slave s p?íznakem zápisu (SLA+W)
 TW_SEND_DATA;                              // odeslat adresu  
	
 TW_WAIT;                                   // po?kat na odezvu TWI
 if ((TW_STATUS) != TW_MT_SLA_ACK){TW_SEND_STOP; return 2;} // Slave nám nedal potvrzení, p?ípadn? nastal jiný problém ? kon?íme komunikaci STOP sekvencí
 TWDR = data;                               // nahrát data která chceme poslat do slave
 TW_SEND_DATA;                              // odeslat data
 
 TW_WAIT;                                   // po?kat na odezvu TWI
 if ((TW_STATUS) != TW_MT_DATA_ACK){TW_SEND_STOP; return 2;} // Slave nám nedal potvrzení, p?ípadn? nastal jiný problém ? kon?íme komunikaci STOP sekvencí
 TWDR = data2;                               // nahrát data která chceme poslat do slave
 TW_SEND_DATA;                              // odeslat data
	
 TW_WAIT;                                   // po?kat na odezvu TWI
 if ((TW_STATUS) != TW_MT_DATA_ACK){TW_SEND_STOP; return 2;} // Slave nám nedal potvrzení, p?ípadn? nastal jiný problém ? kon?íme komunikaci STOP sekvencí
 TWDR = data3;                               // nahrát data která chceme poslat do slave
 TW_SEND_DATA;                              // odeslat data

 TW_WAIT;                                   // po?kat na odezvu TWI
 if ((TW_STATUS) != TW_MT_DATA_ACK){TW_SEND_STOP; return 3;} // Slave nám nedal potvrzení, p?ípadn? nastal jiný problém ? kon?íme komunikaci STOP sekvencí
 TW_SEND_STOP;                              // konec komunikace
 return 0;
}


/*------------------------------------------------------------------------
**  blinkEm - function to blink LED for count passed in
**		Assumes that leds are all on the same port. Maximum delay 
**		is 262ms - using 200x5 delays 1 second for a clock rate of 1MHz.
**		Increase for proportionally for faster clocks. For 4MHZ Clock,
**		the values are 200/4 and 5*4, or 50 and 20.
** ---------------------------------------------------------------------*/
void blinkEm( uint8_t count, uint8_t led)
{
	uint8_t i;
	while (count > 0){
		PORT_LED |= _BV(led);
		for (i=0;i<20;i++)
		{
			_delay_ms(50);
		}
		PORT_LED &= ~_BV(led);
		for (i=0;i<20;i++)
		{
			_delay_ms(50);
		}
		count--;

	}
}