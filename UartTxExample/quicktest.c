#define STRLEN 32
#define F_CPU 1843200UL
#define USART_BAUDRATE 115200
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <util/delay.h>

void sendstring(void);

volatile char out[STRLEN];
volatile unsigned int outp;

volatile unsigned char tx_send;

int main(void) {
  UCSRB |= (1 << TXEN); // enable tx
  UBRRH = (BAUD_PRESCALE >> 8); // set baud
  UBRRL = BAUD_PRESCALE;
  
  sei(); // enable interrupts
  
  tx_send = 0;
  
  strcpy((char*)out, "Hello ");
  sendstring();
  while (tx_send == 1) { }
  strcpy((char*)out, "world");
  sendstring();
  
  while (1) { }

  return 0;
}

void sendstring() {
  outp = 0;
  tx_send = 1;
  UDR = out[outp];
  UCSRB |= (1<<UDRIE);
}

ISR(USART_UDRE_vect) {
  UCSRB &= ~(1<<UDRIE);
  if (out[outp] == '\0') {
    tx_send = 0;
  } else {
    outp++;
    UDR = out[outp];
    UCSRB |= (1<<UDRIE);
  }
}