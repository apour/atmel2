#define BUF_LEN 32
#define F_CPU 16000000UL
#define USART_BAUDRATE 19200
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <util/delay.h>

void sendstring(void);

volatile char out[BUF_LEN];
volatile unsigned int outp;

volatile unsigned char tx_send;

int main(void) {
  UCSRB |= (1 << TXEN); // enable tx
  UBRRH = (BAUD_PRESCALE >> 8); // set baud
  UBRRL = BAUD_PRESCALE;
  
  sei(); // enable interrupts
  
  tx_send = 0;
  
  writeString("Hello ");
  writeString("world");
  
  while (1) 
	{
		_delay_ms(1000);
		writeString("Alexandr");
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

void uartPutTxt(char *data)
{
	uint8_t len = strlen( (const char*) data);
	if (len>BUF_LEN)
	{
		writeString("Problem");
		return;
	}		
	writeString(data);
}

void uartOutDigit(uint8_t v)
{
	if (v<10)
		uartPutc(v+0x30);
	else 
		uartPutc(v+0x41-10);
}