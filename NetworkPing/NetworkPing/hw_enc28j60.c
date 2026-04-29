/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher 
 * Copyright: GPL V2
 * http://www.gnu.org/licenses/gpl.html
 *
 * Based on the enc28j60.c file from the AVRlib library by Pascal Stang.
 * For AVRlib See http://www.procyonengineering.com/
 * Used with explicit permission of Pascal Stang.
 *
 * Title: Microchip ENC28J60 Ethernet Interface Driver
 * Chip type           : ATMEGA88 with ENC28J60
 *********************************************/
#include <avr/io.h>
#include <string.h>
#include "avr_compat.h"
#include "hw_enc28j60.h"
#include "uart.h"

#define F_CPU 12000000UL

//#define LOG_RECEIVED_INFO	1
//#define LOG_RECEIVED_DATA	1
//#define LOG_SENT_DATA		1
#define UART_NL				uartPutc('\r');	\
							uartPutc('\n')	
//
#ifndef ALIBC_OLD
#include <util/delay.h>
#else
#include <avr/delay.h>
#endif

static uint8_t Enc28j60Bank;
static uint16_t NextPacketPtr;
static uint8_t unreleasedPacket;
static uint8_t hangUp;

// set CS to 0 = active
#define CSACTIVE ENC28J60_CONTROL_PORT&=~(1<<ENC28J60_CONTROL_CS)
// set CS to 1 = passive
#define CSPASSIVE ENC28J60_CONTROL_PORT|=(1<<ENC28J60_CONTROL_CS)
//
#define waitspi() while(!(SPSR&(1<<SPIF)))

void xferSPI(uint8_t data)
	{
	SPDR = data;
	while(!(SPSR&(1<<SPIF)));
	}

uint8_t enc28j60ReadOp(uint8_t op, uint8_t address)
	{
    CSACTIVE;
    // issue read command
    SPDR = op | (address & ADDR_MASK);
    waitspi();
    // read data
    SPDR = 0x00;
    waitspi();
    // do dummy read if needed (for mac and mii, see datasheet page 29)
    if(address & 0x80)
		{
        SPDR = 0x00;
        waitspi();
		}
    // release CS
    CSPASSIVE;
    return(SPDR);
	}

void enc28j60WriteOp(uint8_t op, uint8_t address, uint8_t data)
	{
    CSACTIVE;
    // issue write command
    SPDR = op | (address & ADDR_MASK);
    waitspi();
    // write data
    SPDR = data;
    waitspi();
    CSPASSIVE;
	}

void enc28j60ReadBuffer(uint16_t len, uint8_t* data)
	{
	uint8_t nextbyte;
	CSACTIVE;
	if (len != 0) 
		{
		xferSPI(ENC28J60_READ_BUF_MEM);
          
		SPDR = 0x00; 
		while (--len) 
			{
			while (!(SPSR & (1<<SPIF)))
				;
			nextbyte = SPDR;
			SPDR = 0x00;
			*data++ = nextbyte;     
			}
		while (!(SPSR & (1<<SPIF)))
			;
		*data++ = SPDR;    
		}	
	CSPASSIVE;
	}

void enc28j60WriteBuffer(uint16_t len, uint8_t* data)
	{
	CSACTIVE;
	if (len != 0) 
		{
		xferSPI(ENC28J60_WRITE_BUF_MEM);
           
		SPDR = *data++;    
		while (--len) 
			{
			uint8_t nextbyte = *data++;
        	while (!(SPSR & (1<<SPIF)))
				;
			SPDR = nextbyte;
     		};  
		while (!(SPSR & (1<<SPIF)))
			;
		}
	CSPASSIVE;
	}

void enc28j60SetBank(uint8_t address)
	{
    // set the bank (if needed)
    if((address & BANK_MASK) != Enc28j60Bank)
		{
        // set the bank
        enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
        Enc28j60Bank = (address & BANK_MASK);
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, Enc28j60Bank>>5);
		}
	}

uint8_t enc28j60Read(uint8_t address)
	{
    // set the bank
    enc28j60SetBank(address);
    // do the read
    return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);
	}

uint16_t readReg(uint8_t address)
	{
	return enc28j60Read(address) + (enc28j60Read(address+1) << 8);
	}

void enc28j60Write(uint8_t address, uint8_t data)
	{
    // set the bank
    enc28j60SetBank(address);
    // do the write
    enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
	}

void writeReg(uint8_t address, uint16_t data) 
	{
	enc28j60Write(address, data&0xFF);
	enc28j60Write(address+1, data>>8);
	}

// read upper 8 bits
uint16_t enc28j60PhyReadH(uint8_t address)
	{
	// Set the right address and start the register read operation
	enc28j60Write(MIREGADR, address);
	enc28j60Write(MICMD, MICMD_MIIRD);
        //fcpu_delay_us(15);
        _delay_us(15);

	// wait until the PHY read completes
	while(enc28j60Read(MISTAT) & MISTAT_BUSY);

	// reset reading bit
	enc28j60Write(MICMD, 0x00);
	
	return (enc28j60Read(MIRDH));
	}


void enc28j60PhyWrite(uint8_t address, uint16_t data)
	{
    // set the PHY register address
    enc28j60Write(MIREGADR, address);
    // write the PHY data
    enc28j60Write(MIWRL, data);
    enc28j60Write(MIWRH, data>>8);
    // wait until the PHY write completes
    while(enc28j60Read(MISTAT) & MISTAT_BUSY)
		{
        _delay_us(15);
		}
	}

void enc28j60clkout(uint8_t clk)
	{
    //setup clkout: 2 is 12.5MHz:
	enc28j60Write(ECOCON, clk & 0x7);
	}

void enc28j60Init(uint8_t* macaddr)
	{
	hangUp = 0;

	// initialize I/O
    // ss as output:
	ENC28J60_CONTROL_DDR |= 1<<ENC28J60_CONTROL_CS;
	CSPASSIVE; // ss=0
        //	
	DDRB  |= 1<<PB3 | 1<<PB5; // mosi, sck output
	cbi(DDRB,PINB4); // MISO is input
    //
    cbi(PORTB,PB3); // MOSI low
    cbi(PORTB,PB5); // SCK low
	//
	// initialize SPI interface
	// master mode and Fosc/2 clock:
    //SPCR = (1<<SPE)|(1<<MSTR);
	SPCR = (1<<SPE)|(1<<MSTR);//|(1<<SPR0);
    SPSR |= (1<<SPI2X);
	SPSR |= (1<<SPR0);
	
	uartWriteUInt8('B');
	uint8_t rev = enc28j60getrev();
	uartWriteUInt8(rev);
	uartPutc('\r');

	//SPSR = 0;
	// perform system reset
	enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
	
	_delay_ms(2); // errata B7/2
    while (!enc28j60ReadOp(ENC28J60_READ_CTRL_REG, ESTAT) & ESTAT_CLKRDY)
        ; 
		
	//fcpu_delay_ms(50);
	//_delay_ms(50);
	// check CLKRDY bit to see if reset is complete
        // The CLKRDY does not work. See Rev. B4 Silicon Errata point. Just wait.
	//while(!(enc28j60Read(ESTAT) & ESTAT_CLKRDY));
	// do bank 0 stuff
	// initialize receive buffer
	// 16-bit transfers, must write low byte first
	// set receive buffer start address
	NextPacketPtr = RXSTART_INIT;
	unreleasedPacket = 0;
    // Rx start
	writeReg(ERXSTL, RXSTART_INIT);	
	// set receive pointer address
	writeReg(ERXRDPTL, RXSTART_INIT);
	// RX end
	writeReg(ERXNDL, RXSTOP_INIT);
	// TX start
	writeReg(ETXSTL, TXSTART_INIT);
	// TX end
	writeReg(ETXNDL, TXSTOP_INIT);
	// do bank 1 stuff, packet filter:
    // For broadcast packets we allow only ARP packtets
    // All other packets should be unicast only for our mac (MAADR)
    //
    // The pattern to match on is therefore
    // Type     ETH.DST
    // ARP      BROADCAST
    // 06 08 -- ff ff ff ff ff ff -> ip checksum for theses bytes=f7f9
    // in binary these poitions are:11 0000 0011 1111
    // This is hex 303F->EPMM0=0x3f,EPMM1=0x30
	
	
	// from arduino
	//enc28j60Write(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN);
	enc28j60Write(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN|ERXFCON_BCEN);
	
	writeReg(EPMM0, 0x303F);
	writeReg(EPMM0, 0xF9F7);
    //
    //
	// do bank 2 stuff
	// enable MAC receive
	enc28j60Write(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	// bring MAC out of reset
	enc28j60Write(MACON2, 0x00);
	// enable automatic padding to 60bytes and CRC operations
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
	// set inter-frame gap (non-back-to-back)
	writeReg(EPMM0, 0x0C12);
	// set inter-frame gap (back-to-back)
	enc28j60Write(MABBIPG, 0x12);
	// Set the maximum packet size which the controller will accept
    // Do not send packets longer than MAX_FRAMELEN:
	writeReg(MAMXFLL, MAX_FRAMELEN);
	// do bank 3 stuff
    // write MAC address
    // NOTE: MAC address in ENC28J60 is byte-backward
    enc28j60Write(MAADR5, macaddr[0]);
    enc28j60Write(MAADR4, macaddr[1]);
    enc28j60Write(MAADR3, macaddr[2]);
    enc28j60Write(MAADR2, macaddr[3]);
    enc28j60Write(MAADR1, macaddr[4]);
    enc28j60Write(MAADR0, macaddr[5]);
	// no loopback of transmitted frames
	enc28j60PhyWrite(PHCON2, PHCON2_HDLDIS);
	// switch to bank 0
	enc28j60SetBank(ECON1);
	// enable interrutps
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
	// enable packet reception
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
	}

// read the revision of the chip:
uint8_t enc28j60getrev(void)
	{
	return(enc28j60Read(EREVID));
	}

// link status
uint8_t enc28j60linkup(void)
	{
    // bit 10 (= bit 3 in upper reg)
    if (enc28j60PhyReadH(PHSTAT2) && 4)
		{
        return(1);
		}
    return(0);
	} 

uint8_t transmit_status_vector[7];
	
void enc28j60PacketSend(uint16_t len, uint8_t* packet)
	{
#ifdef LOG_SENT_DATA
	uint16_t i=0;
	uartPutTxt("s:");
	for (i=0; i<len; i++)
		{
		uartWriteUInt8(packet[i]);
		}
	UART_NL;
#endif			
	// Set the write pointer to start of transmit buffer area
	writeReg(EWRPTL, TXSTART_INIT);
	// Set the TXND pointer to correspond to the packet size given
	writeReg(ETXNDL, TXSTART_INIT+len);
	// write per-packet control byte (0x00 means use macon3 settings)
	enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);
	// copy the packet into the transmit buffer
	enc28j60WriteBuffer(len, packet);
	
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRST);
    enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRST); 
		
	// clear EIR.TXIF and EIR.TXERIF
	enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, EIR, EIR_TXERIF|EIR_TXIF);
	// set EIE.TXIF and EIE.INTE
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_TXIE);
	
	// initiate transmission
	// send the contents of the transmit buffer onto the network
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);

	//_delay_ms(1);
	_delay_us(200);
	
	while (enc28j60ReadOp(ENC28J60_READ_CTRL_REG, ECON1) & ECON1_TXRTS)
		{
        asm("nop");
		}		
		
	while ((enc28j60Read(EIR) & (EIR_TXIF | EIR_TXERIF)) == 0)
		{
		_delay_us(1);
		}		
	
#ifdef LOG_SENT_DATA
		uint8_t etxndl = 0;
		uint8_t etxndh = 0;
		etxndl = enc28j60Read(ETXNDL);
		etxndh = enc28j60Read(ETXNDH);
		uartPutTxt("Pos:");
		uartWriteUInt8(etxndh);
		uartWriteUInt8(etxndl);
		UART_NL;		
		
		uint16_t readPtr = etxndh<<8;
		readPtr|= etxndl;
		
		readPtr++;
		writeReg(ERDPT, readPtr);
		
		enc28j60ReadBuffer(7, &transmit_status_vector[0]);
		
		uartPutTxt("TXSTAT:");
		for (int i=0; i<7; i++)
			{
			uartWriteUInt8(transmit_status_vector[i]);	
			}
		UART_NL;
		
		uint8_t estat = enc28j60ReadOp(ENC28J60_READ_CTRL_REG, ESTAT);
		if ( (estat&ESTAT_TXABRT) != 0)
			{
			uartPutTxt("Transmit aborted\r\n");
			}
		uint8_t eir = enc28j60Read(EIR);
		if ( (eir&EIR_TXERIF) != 0)
			{
			uartPutTxt("Transmit error\r\n");
			}
#endif
	
	// clear EIR.TXIF
	enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, EIR, EIR_TXIF);
	}

uint16_t enc28j60PacketReceive2(uint16_t maxlen, uint8_t* packet)
	{
	uint16_t len=0;
	uint8_t epkctn;
	
	if (unreleasedPacket == 1) 
		{
#ifdef LOG_RECEIVED_INFO			
		uartPutc('U');
#endif		
        if (NextPacketPtr == 0) 
			{
			writeReg(ERXRDPTL, RXSTOP_INIT);
#ifdef LOG_RECEIVED_INFO			
			uartPutc('R');
#endif			
			}			
        else
			{
			writeReg(ERXRDPT, NextPacketPtr-1);
			waitspi();
#ifdef LOG_RECEIVED_INFO
			uartPutTxt("W:");
			uartWriteUInt16(NextPacketPtr-1);
			UART_NL;
#endif			
			}			
        unreleasedPacket = 0;
		}
		
	epkctn = enc28j60Read(EPKTCNT);
	if( epkctn > 0)
		{
#ifdef LOG_RECEIVED_INFO		
		// read EIR
		uint8_t eir = enc28j60Read(EIR);
		uartPutTxt("RXEIR:");
		uartWriteUInt8(eir);	
		UART_NL;		
		
		// read ESTAT
		uint8_t estat = enc28j60Read(ESTAT);
		uartPutTxt("RXESTAT:");
		uartWriteUInt8(estat);	
		UART_NL;		
		
		uartPutc('E');
		uartPutc(':');
		uartWriteUInt8(epkctn);
		UART_NL;
#endif		
		// Set the read pointer to the start of the received packet
		writeReg(ERDPT, NextPacketPtr);
#ifdef LOG_RECEIVED_INFO				
		uartPutTxt("N1:");
		uartWriteUInt16(NextPacketPtr);
		UART_NL;
#endif		
		_delay_us(50);
		
		struct {
            uint16_t nextPacket;
            uint16_t byteCount;
            uint16_t status;
        } header; 
		
		enc28j60ReadBuffer(sizeof(header), (uint8_t*) &header);

#ifdef LOG_RECEIVED_INFO		
		{
		uint8_t i=0;
		uint8_t* p = &header;
		while (i<sizeof(header))
			{
			uartWriteUInt8(*p);
			i++;
			p++;
			}
		}			
#endif
		
		len = header.byteCount;
		NextPacketPtr = header.nextPacket;

#ifdef LOG_RECEIVED_INFO		
		uartPutTxt("N2:");
		uartWriteUInt16(NextPacketPtr);
		UART_NL;
		
		uartPutTxt("L:");
		uartWriteUInt16( len );
		UART_NL;
		//
		////len-= 4; //remove the CRC count
		//
		uartPutTxt("RX:");
		uartWriteUInt16(header.status);
		UART_NL;
#endif

		// limit retrieve length
        if (len>maxlen-1)
			{
            len=maxlen-1;
			}
        // check CRC and symbol errors (see datasheet page 44, table 7-3):
        // The ERXFCON.CRCEN is set by default. Normally we should not
        // need to check this.
        if ((header.status & 0x80)==0)
			{
            // invalid
            len=0;
#ifdef LOG_RECEIVED_INFO			
			uartPutTxt("ERR");
			UART_NL;
#endif			
			}			
        else
			{
            // copy the packet from the receive buffer
            enc28j60ReadBuffer(len, packet);
			packet[len] = 0;
			
#ifdef LOG_RECEIVED_DATA
			{
			// dump received data
			uartPutTxt("D:");
			uint16_t i=0;
			while (i<len)
				{
				uartWriteUInt8(packet[i]);
				i++;
				}
			UART_NL;
			UART_NL;
			}			
#endif
			}

		unreleasedPacket = 1;
		
		// decrement the packet counter indicate we are done with this packet
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
		}
	return(len);
	}

uint8_t isHangUp(void)
	{
	return hangUp;
	}	
