/*
 * NetworkTest.c
 *
 * Created: 2.10.2016 20:32:06
 *  Author: Ales
 */ 

#define F_CPU   12000000L    /* evaluation board runs on 16MHz */

#include <avr/interrupt.h>
#include <util/delay.h>
#include "aux_globals.h"
#include "avr_compat.h"
#include "hw_enc28j60.h"
#include "net.h"

/* Measure temperature - Begin - input PC3 */
#define LOG_TO_UART	1
#define TEMP_INPUT_PIN	3
#define TEMP_0		559
#define TEMP_100	764
#define TEMP_1_DEGREE	(TEMP_100-TEMP_0)/100.0
static unsigned int voltage=0x66;

// please modify the following two lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x03,0x24};
static uint8_t myip[4] = {192,168,1,130}; 
// listen port for tcp/www (max range 1-254)
#define MYWWWPORT 80
// working buffer
#define BUFFER_SIZE 500
static uint8_t buf[BUFFER_SIZE+1];
// Global counters
static int nPingCount = 0, nAccessCount = 0; 
	
int main(void)
{
	odDebugInit();
	
	// reset enc28j60
	DDRB  |= 1<<PB0;	// PB0 as output
	
	cbi(PORTB,PB0); // set PB0 low
	_delay_ms(50);
	sbi(PORTB,PB0); // set PB0 hi
	
	uartPutText("\r\nInit->: ", 8);

	//initialize enc28j60
    enc28j60Init(mymac); 
	fcpu_delay_ms(10);
	
	uint8_t rev = enc28j60getrev();
	uartWriteUInt8(rev);
	uartPutc('\r');
	uartPutc('\n');
	

	fcpu_delay_ms(100);

    //init the ethernet/ip layer:
	init_udp_or_www_server(mymac,myip);
	www_server_port(MYWWWPORT);
	
	fcpu_delay_ms(100); 
		
	uint16_t plen, dat_p, res;
    while(1){
	
		//voltage = 0;
		ADCW = 0;
		ADMUX = TEMP_INPUT_PIN;
		ADCSRA |= (1<<ADSC); // Start conversion
		while (ADCSRA & (1<<ADSC)); // wait for conversion to complete
		//voltage = ADCW; 

		plen = enc28j60PacketReceive2(BUFFER_SIZE, buf);
		
		res = packetloop_arp_icmp_tcp(buf, plen, voltage);
		if (res == 0)
		{
			continue;
		}			
		memset(buf, 0, BUFFER_SIZE);
		
		//---------------------------------------------
    } // while
    return (0); 
	
    //while(1)
    //{
		//fcpu_delay_ms(500);
		////uartPutText("Ahojda", 6);
    //}
}