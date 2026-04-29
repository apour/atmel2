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

/****************************************************************/
/*    A N T I V I R   M U S T   B E   D I S A B L E D   !!!!	*/
/****************************************************************/

// Functions only with TPLINK WIFI ROUTER !!!

#define LOG_TO_UART	1
#define TEMP_INPUT_PIN	3
static unsigned int voltage=0x66;

// please modify the following two lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x03,0x24};
static uint8_t myip[4] = {192,168,1,130}; 
// listen port for tcp/www (max range 1-254)
#define MYWWWPORT 80
// working buffer
#define BUFFER_SIZE 700
static uint8_t buf[BUFFER_SIZE+1];
// Global counters
//static int nPingCount = 0, nAccessCount = 0; 

const char page[] PROGMEM =
"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body>B</body></html>";
	
const char pageHeader[] PROGMEM = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body>MAC: ";
const char macLabel[] PROGMEM = "MAC: ";
const char pageFooter[] PROGMEM = "</body></html>";
const char ipLabel[] PROGMEM = "IP: ";
	
const char smallText[] = "ABC";

const char page2[] =
"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body>B</body></html>";	

static char tempValueBuf[20];
static unsigned short r;
static uint8_t i;

//init adc
void init_ADC()   
	{ 
	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS0);
	}
		
void dumpStatistic()
{
	uartPutc('-');
	uartPutc('-');
	uartPutc('\r');
	uartPutc('\n');
	
	uint8_t rev;
	rev = enc28j60Read(EPKTCNT);
	uartPutc('E');
	uartPutc('P');
	uartPutc('K');
	uartPutc('T');
	uartPutc('C');
	uartPutc('N');
	uartPutc('T');
	uartPutc(':');
	uartWriteUInt8(rev);
	uartPutc('\r');
	uartPutc('\n');
		
	rev = enc28j60Read(ESTAT);
	uartPutc('E');
	uartPutc('S');
	uartPutc('T');
	uartPutc('A');
	uartPutc('T');
	uartPutc(':');
	uartWriteUInt8(rev);
	uartPutc('\r');
	uartPutc('\n');
			
	rev = enc28j60Read(EIR);
	uartPutc('E');
	uartPutc('I');
	uartPutc('R');
	uartPutc(':');
	uartWriteUInt8(rev);
	uartPutc('\r');
	uartPutc('\n');
			
	rev = enc28j60Read(EIE);
	uartPutc('E');
	uartPutc('I');
	uartPutc('E');
	uartPutc(':');
	uartWriteUInt8(rev);
	uartPutc('\r');
	uartPutc('\n');
	
	uartPutc('-');
	uartPutc('-');
	uartPutc('\r');
	uartPutc('\n');
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
		
	uint16_t counter=0;
	uint16_t plen, dat_p, res;
    while(1){
	
		voltage = 0;
		plen = enc28j60PacketReceive2(BUFFER_SIZE, buf);
		
		if (plen > 0)
			{
			counter = 0;
			}
		counter++;
		if (counter == 65000)
			{
			//rev = enc28j60getrev();
			//uartWriteUInt8(rev);
			//uartPutc('\r');
			//
			//dumpStatistic();
			//
			//uartPutc('I');
			//uartPutc('\r');
			//uartPutc('\n');
			//enc28j60Init(mymac); 
			//fcpu_delay_ms(10);
	
			counter = 0;
			}
	
		res = packetloop_arp_icmp_tcp(buf, plen, voltage);
		if (res == 0)
		{
			continue;
		}			
		
		if (res < 34)
			{
			uartPutText("To short data\r\n", 15);
			continue;
			}
		uartPutText("WWW RESPONSE\r\n", 14);
		
		int i=0;
		while (i<plen)
			{
			uartWriteUInt8(buf[i]);
			i++;
			}
		uartPutText("\r\n", 2);
		
		_delay_us(200);
		//char bufCopy[6];
		//memcpy(bufCopy, buf, 6);
		//
		//
		//char ipCopy[10];
		//memcpy(ipCopy, buf+26, 4);

//
		//char tempValueBuf[50];
        //char tempData[256];
		//tempData[0] = '\0';
		//strcat(tempData, "\r\n"
			//"<html>"
			  //"<body>"
				//"Ahojda jak to jde? ABC"
			  //"</body>"
			//"</html>");
		//uartPutText("CALC LEN\r\n", 10);
//
		//unsigned long dataLen = strlen(tempData);
//
		//memset(tempValueBuf, 0, 50);
		//unsigned short r;
		//r = convertUInt8ToText(tempValueBuf, dataLen, 0);
		//tempValueBuf[r] = '\0';
		//uartPutText(tempValueBuf, r+1);	
        //uartPutText("\r\n", 2);				
		//
		//memcpy(buf+0x36, page, sizeof page);
		//plen = sizeof page - 1; 
		//plen = fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\n"
			//"Content-Type: text/html\r\n"
			////"Content-Length: 46\r\n"
			//));
		////memcpy(buf[plen], tempValueBuf, r);
		////plen+= r;
//
		//plen = fill_tcp_data(buf, plen, 
		    ////"\r\n"
			//"\r\n"
			//"<html>"
			  //"<body>"
				//"Ahojda jak to jde? ");
			   ////"</body>"
			////"</html>");
		//plen = fill_tcp_data(buf, plen, "Request from MAC: ");
		//
		//plen = fill_tcp_data(buf, plen, 
			  //"</body>"
			//"</html>");
						//
		//plen = fill_tcp_data_len(buf, 0, page2, sizeof page2 - 1);						
		plen = fill_tcp_data_p(buf,0,pageHeader);
		res = sizeof pageHeader-1;
		//plen = fill_tcp_data_p(buf,res,macLabel);
		//res+= sizeof macLabel-1;
		
		//tempValueBuf[0] = 'D';
		//tempValueBuf[1] = 'E';
		//tempValueBuf[2] = 'F';
		//fill_tcp_data_len(buf, res, tempValueBuf, 3);
		//res+= 3;
		//fill_tcp_data_len(buf, res, "ABC", 3);
		//res+= 3;
		//buf[TCP_CHECKSUM_L_P+3+res]='X';
		//res++;
		
		//unsigned short r;
		//tempValueBuf[0] = '1';
		//tempValueBuf[1] = '4';
		//for (i=6; i<12; i++)
			//{
			//r = convertUInt8ToHexText(&tempValueBuf, buf[i]);
			//buf[TCP_CHECKSUM_L_P+3+res]=tempValueBuf[0]; res++;
			//buf[TCP_CHECKSUM_L_P+3+res]=tempValueBuf[1]; res++;
			////res+= r;
			////if (i!=11)
				////{
				////buf[TCP_CHECKSUM_L_P+3+res]=':'; res++;
				////}								
			//}
						
		//buf[TCP_CHECKSUM_L_P+3+res]='I'; res++;
		//buf[TCP_CHECKSUM_L_P+3+res]='P'; res++;
		//plen = fill_tcp_data_p(buf,res,ipLabel);
		//res+= sizeof ipLabel-1;
		plen = fill_tcp_data_p(buf,res,pageFooter);
			
		uartPutText("Size: ", 6);
		uartWriteUInt8(plen);
		uartPutText("\r\n", 2);
		
		//dumpStatistic();
		// set bits to EIE
		//rev = enc28j60Read(EIE);
		//if (rev == 0x0)
			//{	
			//// set INTIE, PKTIE bits		
			//enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
			//uartPutText("Set enable INT", 14);
			//uartPutText("\r\n", 2);
			//dumpStatistic();
			//}
			
		www_server_reply(buf, plen);

		
		// ----- BEGIN
		//plen = fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\n"
			//"Content-Type: text/html\r\n"
			////"Retry-After: 600\r\n"
			////"Content-Length: 45\r\n"
			//"\r\n"
			//"<html>"
			  //"<body>"
				//"Ahojda jak to jde?"
			  //"</body>"
			//"</html>"));
						////
		//www_server_reply(buf, plen);
		// ----- END
		
		//plen = fill_tcp_data_p(buf,0,PSTR("HTTP/1.1 200 OK\r\n"
			//"Content-Type: text/html\r\n"
			//"Retry-After: 600\r\n"
			//"\r\n"
			//"<head><title>Response</title></head>"
			//"<body>"));
			//
				//
		//plen = fill_tcp_data(buf, plen, "Request from MAC: ");
									//
						//// MAC Address
						//char tempValueBuf[50];
						//unsigned long v;
						//unsigned short r;
						//
						//char* outBuf = tempValueBuf;
						//for (char i=0; i<6; i++)
						//{
							//r = convertUInt8ToHexText(outBuf, bufCopy[i]);
							//outBuf+= r;
							//if (i!=5)
							//{
								//*outBuf=':';
								//outBuf++;
							//}								
						//}
						//*outBuf = '\0';
						//plen = fill_tcp_data(buf, plen, tempValueBuf);
						//
						//// IP Address
						//plen = fill_tcp_data(buf, plen, "<br>IP: ");
						//
						////memcpy(bufCopy, buf+26, 6);
						//outBuf = tempValueBuf;
						//for (char i=0; i<4; i++)
						//{
							////r = convertUInt8ToHexText(outBuf, ipCopy[i]);
							//v = ipCopy[i];
							//r = convertUInt8ToText(outBuf, v, 0);
							//outBuf+= r;
							//if (i!=3)
							//{
								//*outBuf='.';
								//outBuf++;
							//}								
						//}
						//*outBuf = '\0';
						//plen = fill_tcp_data(buf, plen, tempValueBuf);
								//
						//plen = fill_tcp_data(buf, plen, "</body></html>");
						//www_server_reply(buf, plen);
								//
		//free(ipCopy);
		
		//---------------------------------------------
    } // while
    return (0); 
}