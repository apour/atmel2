/*
 * NetworkPing.c
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
#include "ip_arp_udp_tcp.h"
#include "uart.h"
#include "conversion.h"

/*************************************************************************/
/*	Must be used external current source - consumption approx 0.2A	     */
/*************************************************************************/

#define UART_NL				uartPutc('\r');	\
							uartPutc('\n')	

#define LOG_TO_UART	1
#define TEMP_INPUT_PIN	3
static unsigned int voltage=0x66;

// please modify the following two lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x03,0x24};
static uint8_t myip[4] = {192,168,1,131}; 
// listen port for tcp/www (max range 1-254)
#define MYWWWPORT 80
// working buffer
#define BUFFER_SIZE 700
static uint8_t buf[BUFFER_SIZE+1];
//#define RECEIVE_DATA_SIZE	192
#define RECEIVE_DATA_SIZE	128
static uint8_t received_data[RECEIVE_DATA_SIZE];

const char pageHeader[] PROGMEM =
"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body>";
const char pageMACLabel[] PROGMEM =
"MAC: ";
const char pageIPLabel[] PROGMEM =
" IP: ";
const char pageContent[] PROGMEM =
"</body></html>";
const char pageBR[] PROGMEM =
"<BR>";
	
static char tempValueBuf[20];

//init adc
void init_ADC()   
	{ 
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS0);
	}
	
void dumpStatistic()
	{
	uartPutTxt("--");
	UART_NL;
	
	uint8_t rev;
	rev = enc28j60Read(EPKTCNT);
	uartPutTxt("EPKTCNT:");
	uartWriteUInt8(rev);
	UART_NL;
		
	rev = enc28j60Read(ESTAT);
	uartPutTxt("ESTAT:");
	uartWriteUInt8(rev);
	UART_NL;
			
	rev = enc28j60Read(EIR);
	uartPutTxt("EIR:");
	uartWriteUInt8(rev);
	UART_NL;
			
	rev = enc28j60Read(EIE);
	uartPutTxt("EIE:");
	uartWriteUInt8(rev);
	UART_NL;
	
	uartPutTxt("--");
	UART_NL;
	}	
		
int main(void)
{
	odDebugInit();
	init_ADC();
	
	// reset enc28j60
	DDRB  |= 1<<PB0;	// PB0 as output
	
	//DDRC |= 1<<PC0;	// PC0 as output
	//cbi(PORTC,PC0); // set PC0 lo
	
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
	if (rev == 6)
		{
		// switch on led on PC0
		sbi(PORTC,PC0); // set PC0 lo
		}

	fcpu_delay_ms(100);
	
    //init the ethernet/ip layer:
	init_udp_or_www_server(mymac,myip);
	www_server_port(MYWWWPORT);
	
	fcpu_delay_ms(100); 
		
	uint16_t plen, res, dataLen;
    while(1){
		voltage = 0;
		plen = enc28j60PacketReceive2(BUFFER_SIZE, buf);
		
		res = packetloop_arp_icmp_tcp(buf, plen);
		if (res == 0)
		{
			continue;
		}	

		dataLen = plen-res-4;
		if (dataLen>RECEIVE_DATA_SIZE)
			dataLen = RECEIVE_DATA_SIZE;
		for (int i=0; i<dataLen; i++)
			{
			received_data[i] = buf[res+i];
			}
		
#ifdef LOG_TO_UART
		uartPutTxt("Pos: ");	
		uartWriteUInt16(res);
		uartWriteUInt16(plen);
		UART_NL;
		uartPutText( (char*) &buf[res], plen-res-4);
		UART_NL;
		uartPutText( (char*) received_data, dataLen);
#endif		
		
		plen = fill_tcp_data_p(buf,0,pageHeader);
		res = sizeof pageHeader-1;
	
		plen = fill_tcp_data_p(buf,res,pageMACLabel);		
		res+= sizeof pageMACLabel-1;
		
		unsigned short r;
		for (uint8_t i=6; i<12; i++)
			{
			r = convertUInt8ToHexText( (char*) &tempValueBuf, buf[i]);
			buf[TCP_CHECKSUM_L_P+3+res]=tempValueBuf[0]; res++;
			buf[TCP_CHECKSUM_L_P+3+res]=tempValueBuf[1]; res++;
			if (i!=11)
				{
				buf[TCP_CHECKSUM_L_P+3+res]=':'; res++;
				}								
			}
			
		plen = fill_tcp_data_p(buf,res,pageIPLabel);		
		res+= sizeof pageIPLabel-1;
		
		for (char i=0; i<4; i++)
			{
			r = convertUInt8ToHexText( (char*) &tempValueBuf, buf[26+i]);
			buf[TCP_CHECKSUM_L_P+3+res]=tempValueBuf[0]; res++;
			buf[TCP_CHECKSUM_L_P+3+res]=tempValueBuf[1]; res++;
			if (i!=3)
				{
				buf[TCP_CHECKSUM_L_P+3+res]='.'; res++;
				}								
			}
		
		plen = fill_tcp_data_p(buf,res,pageBR);		
		res+= sizeof pageBR-1;

#ifdef LOG_TO_UART
		uartPutTxt("PosXXX: ");	
		uartWriteUInt16(res);
#endif				

		for (int i=0; i<dataLen && TCP_CHECKSUM_L_P+3+res<BUFFER_SIZE; i++)
			{
			buf[TCP_CHECKSUM_L_P+3+res]=received_data[i]; res++;
			}		

#ifdef LOG_TO_UART
		uartPutTxt("Pos2: ");	
		uartWriteUInt16(res);		
#endif		
			
		plen = fill_tcp_data_p(buf,res,pageContent);
					
		www_server_reply(buf, plen);
    } // while
    return (0); 
}
