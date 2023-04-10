#define BUF_LEN 64
#define F_CPU 16000000UL
#define USART_BAUDRATE 19200
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <util/delay.h>

void sendstring(void);

char out[BUF_LEN];
volatile unsigned int outp;
volatile unsigned int inp;
volatile unsigned char tx_send;

void flush();
void writeString(char* data);
void uartPutTxt(char *data);
void uartWriteUInt8(uint8_t v);
void uartWriteUInt16(uint16_t v);
void uartWriteUInt32(uint8_t* v);

int main(void) {
  UCSRB |= (1 << TXEN); // enable tx
  UBRRH = (BAUD_PRESCALE >> 8); // set baud
  UBRRL = BAUD_PRESCALE;
  
  sei(); // enable interrupts
  
  tx_send = 0;
  inp = 0;
  
  writeString("Hello ");
  writeString("world");
  // flush();

  uint8_t idx=0;
  uint16_t idx16=0;
  uint32_t idx32=0;
  while (1) 
	{
    //idx32=0x12345678;
		_delay_ms(1000);
    uartPutTxt("Test");
    uartPutTxt("A: ");
    uartWriteUInt8(idx);
    uartPutTxt(" B: ");
    uartWriteUInt16(idx16);
    uartPutTxt(" C: ");
    uint8_t* p = (uint8_t*) &idx32;
    uartWriteUInt32(p);
    uartPutTxt(" Text after data");
    uartPutTxt("\r\n");

    //writeString("ahojda");
		// uartPutTxt("Alexandr");
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


