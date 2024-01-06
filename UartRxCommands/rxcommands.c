#define RX_BUF_LEN  16
#define F_CPU 12000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include "uart.h"

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

// limit's values
unsigned int gridOffLimit;
unsigned int gridOnLimit;
unsigned int changeOffLimit;

void WriteToEEPROM(unsigned int value, uint16_t address)
    {   
    eeprom_write_word((uint16_t*)address, value);
    }

unsigned int ReadEEPROM(unsigned int address)
    {
    unsigned int temp = eeprom_read_word((uint16_t*)address);
    return temp;
    }

unsigned int getValueFromRxBuffer()
  {
  unsigned int res = 0;
  res = (rxBuffer[1] - 0x30) * 100;
  res+= (rxBuffer[2] - 0x30) * 10;
  res+= (rxBuffer[3] - 0x30);
  return res;
  }


int main(void) {
  
  rxByteCount = 0;
  rxState = tIdle;

  DDRB=0xFF;
  PORTB=0xFF;
  sei(); // enable interrupts
  
  uart_init();

  gridOnLimit = ReadEEPROM(2);
  gridOffLimit = ReadEEPROM(4);
  changeOffLimit = ReadEEPROM(6);

  uartPutTxt("Limits: ");
  _delay_ms(100);
  uartPutTxt("GridOn: ");
  _delay_ms(100);
  uartWriteUInt16(gridOnLimit);
  uartPutTxt("\r\n");
  _delay_ms(500);

  uartPutTxt("GridOff: ");
  _delay_ms(100);
  uartWriteUInt16(gridOffLimit);
  uartPutTxt("\r\n");
  _delay_ms(500);

  uartPutTxt("ChargeOff: ");
  _delay_ms(100);
  uartWriteUInt16(changeOffLimit);
  uartPutTxt("\r\n");
  _delay_ms(500);

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

    uart_flush();
    idx++;
    idx16=idx<<1;
    idx32=idx*idx16;
	}

  return 0;
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

