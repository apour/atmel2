#define BUF_LEN 64
#define RX_BUF_LEN  16
#define F_CPU 12000000UL
#define USART_BAUDRATE 1200
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <util/delay.h>
#include <avr/eeprom.h>

void sendstring(void);

char out[BUF_LEN];
volatile unsigned int outp;
volatile unsigned int inp;
volatile unsigned char tx_send;
volatile unsigned int rxByteCount;
unsigned char rxByte;
char rxBuffer[RX_BUF_LEN];
unsigned int tempInt;

enum receiveState
{
  tIdle = 0,
  tUnknownCommand = 1,
  tHelp = 2,
  tGridOnLimit = 3,
  tGridOffLimit = 4,
  tChargeOffLimit = 5
} rxState;

void flush();
void writeString(char* data);
void uartPutTxt(char *data);
void uartWriteUInt8(uint8_t v);
void uartWriteUInt16(uint16_t v);
void uartWriteUInt32(uint8_t* v);

void WriteToEEPROM(unsigned int value, uint16_t address)
    {   
    eeprom_write_word((uint16_t*)address, value);
    }

// unsigned int ReadEEPROM(unsigned int address)
//     {
//     unsigned int temp = eeprom_read_word((unsigned char*)address);
//     return temp;
//     }

unsigned int getValueFromRxBuffer()
  {
  unsigned int res = 0;
  res = (rxBuffer[1] - 0x30) * 100;
  res+= (rxBuffer[2] - 0x30) * 10;
  res+= (rxBuffer[3] - 0x30);
  return res;
  }


int main(void) {
  UCSRB |= (1 << TXEN) | (1 << RXEN)| (1 << TXCIE) | (1 << RXCIE); // enable tx, rx
  UBRRH = (BAUD_PRESCALE >> 8); // set baud
  UBRRL = BAUD_PRESCALE;
  UCSRC=(1<<URSEL)|(0<<UMSEL)|(0<<UPM1)|(0<<UPM0)|
        (0<<USBS)|(0<<UCSZ2)|(1<<UCSZ1)|(1<<UCSZ0); 
  rxByteCount = 0;
  rxState = tIdle;

  DDRB=0xFF;
  PORTB=0xFF;
  sei(); // enable interrupts
  
  tx_send = 0;
  inp = 0;
  
  writeString("Hello ");
  writeString("world");

  uint8_t idx=0;
  uint16_t idx16=0;
  uint32_t idx32=0;
  while (1) 
	{
		_delay_ms(1000);
    PORTB=0xFF;
    // if (rxByteCount>0)
    // {
    //   uartPutTxt("R: ");  
    //   uartWriteUInt8(rxByte);
    //   uartWriteUInt16(rxByteCount);
    //   uartWriteUInt8(rxState);
    //   rxByteCount = 0;
    // }

    switch (rxState)
    {
      case tIdle:
        uartPutTxt("X");
        rxByteCount = 0; 
        break;
      case tUnknownCommand:
        uartPutTxt("UknownCommand");
        rxByteCount = 0;
        rxState = tIdle;
        break;
      case tHelp:
        uartPutTxt("Gxxx - GridOnLimit");
        uartPutTxt(", Hxxx - GriddOffLimit");
        uartPutTxt(", Dxxx - ChargeOffLimit\r\n");
        rxByteCount = 0;
        rxState = tIdle;
        break;
      case tGridOnLimit:
        uartPutTxt("GridOnLimit");
        if (rxByteCount == 4)
        {
          tempInt = getValueFromRxBuffer();
          uartWriteUInt16(tempInt);
          WriteToEEPROM(tempInt, 2);
        }
        rxByteCount = 0;
        rxState = tIdle;
        break;
      case tGridOffLimit:
        uartPutTxt("GridOffLimit");
        if (rxByteCount == 4)
        {
          tempInt = getValueFromRxBuffer();
          uartWriteUInt16(tempInt);
          WriteToEEPROM(tempInt, 4);
        }
        rxByteCount = 0;
        rxState = tIdle;
        break;
      case tChargeOffLimit:
        uartPutTxt("ChargeOffLimit");
        if (rxByteCount == 4)
        {
          tempInt = getValueFromRxBuffer();
          uartWriteUInt16(tempInt);
          WriteToEEPROM(tempInt, 6);
        }
        rxByteCount = 0;
        rxState = tIdle;
        break;
    }
    /*uartPutTxt("Test");
    uartPutTxt("A: ");
    uartWriteUInt8(idx);
    uartPutTxt(" B: ");
    uartWriteUInt16(idx16);
    uartPutTxt(" C: ");
    uint8_t* p = (uint8_t*) &idx32;
    uartWriteUInt32(p);
    uartPutTxt(" Text after data");
    uartPutTxt("\r\n");*/

    flush();
    idx++;
    idx16=idx<<1;
    idx32=idx*idx16;
	}

  return 0;
}

void writeString(char* data)
{
  while (tx_send == 1) 
  {
	  // wait for send previous chars
	  _delay_ms(1);  
  }
  strcpy(out, data);
  sendstring();
}

void sendCurChar()
{
	if (out[outp] != '\0')
		UDR = out[outp];
	UCSRB |= (1<<UDRIE);
}

void sendstring() {  
  outp = 0;
  tx_send = 1;
  sendCurChar();
}

ISR(USART_UDRE_vect) {
  UCSRB &= ~(1<<UDRIE);
   if (out[outp] == '\0') {
    tx_send = 0;
  } else {
    outp++;
	sendCurChar();
  }
}

ISR(USART_TXC_vect)
{

}

ISR(USART_RXC_vect)
{
  PORTB=1;
	rxByte = UDR;
  UCSRA&=(1<<RXC);
  switch (rxState)
  {
    case tIdle:
      if (rxByte == '?')
      {
        rxState = tHelp;
      }
      else if (rxByte == 'G')
      {
        rxState = tGridOnLimit;
        rxBuffer[rxByteCount] = rxByte;
      }
      else if (rxByte == 'H')
      {
        rxState = tGridOffLimit;
        rxBuffer[rxByteCount] = rxByte;
      } 
      else if (rxByte == 'D')
      {
        rxState = tChargeOffLimit;
        rxBuffer[rxByteCount] = rxByte;
      }
      else
      {
        rxState = tUnknownCommand;
      }
      break;
    case tGridOnLimit:
    case tGridOffLimit:
    case tChargeOffLimit:
      if (rxByte < '0' || rxByte > '9')
      {
        rxState = tUnknownCommand;
        break;
      }
      rxBuffer[rxByteCount] = rxByte;
      break;
    default:
      //rxState = tUnknownCommand;
      break;
  } 
  rxByteCount++;
}

// use only following public functions
void flush()
{
   out[inp] = '\0';
   sendstring();
   inp=0;  
}

void uartPutTxt(char *data)
{
	uint8_t len = strlen( (const char*) data);
	if (inp+len>BUF_LEN)
	{
    out[inp] = '!'; inp++;
    flush();
		return;
	}		
  while (*data != '\0')
  {
     out[inp] = *data;
     data++;
     inp++;
  }
}

void uartOutDigit(uint8_t v)
{
	if (v<10)
		out[inp]=(v+0x30);
	else 
		out[inp]=(v+0x41-10);
    inp++;
}

void uartWriteUInt8(uint8_t v)
{
	uint8_t temp = v>>4;
	uartOutDigit(temp);
	temp = v&0xF;
	uartOutDigit(temp);
}	

void uartWriteUInt16(uint16_t v)
{
	uint8_t hi = v>>8;
	uartWriteUInt8(hi);
	hi = v&0xFF;
	uartWriteUInt8(hi);
}	

void uartWriteUInt32(uint8_t* v)
{
	v+=3;
	uint8_t hi = *v;
	uartWriteUInt8(hi);
	v--;
	
	hi = *v;
	uartWriteUInt8(hi);
	v--;

	hi = *v;
	uartWriteUInt8(hi);
	v--;
	
	hi = *v;
	uartWriteUInt8(hi);
}	


