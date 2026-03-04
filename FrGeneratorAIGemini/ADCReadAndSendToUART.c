#define F_CPU 10000000UL // 10 MHz

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#define BAUD 9600
#define MY_UBRR F_CPU/16/BAUD-1

void UART_Init(unsigned int ubrr) {
    // Set baud rate
    UBRRH = (unsigned char)(ubrr >> 8);
    UBRRL = (unsigned char)ubrr;
    // Enable receiver and transmitter
    UCSRB = (1 << RXEN) | (1 << TXEN);
    // Set frame format: 8 data bits, 1 stop bit
    UCSRC = (1 << URSEL) | (3 << UCSZ0);
}

void UART_Transmit(unsigned char data) {
    // Wait for empty transmit buffer
    while (!(UCSRA & (1 << UDRE)));
    // Put data into buffer, sends the data
    UDR = data;
}

void UART_SendString(const char* str) {
    while (*str) {
        UART_Transmit(*str++);
    }
}

void ADC_Init() {
    // Set reference voltage to AVCC and left adjust ADC result
    ADMUX = (1 << REFS0);
    // Enable ADC and ADC interrupt, set prescaler to 64
    ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1);
}

void ADC_Start() {
    // Start conversion by setting ADSC
    ADCSRA |= (1 << ADSC);
}

ISR(ADC_vect) {
    // Read ADC value
    uint16_t adc_value = ADC; // ADC value is now in ADC register

    // Convert ADC value to string
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%u\r\n", adc_value);
    
    // Send the ADC value over UART
    uart_sendString(buffer);    
}

int main(void) {
    UART_Init(MY_UBRR);
    ADC_Init();
    sei(); // Enable global interrupts

    // Start the first ADC conversion
    ADC_Start();

    while (1) {
        // Main loop can do other tasks or sleep
        _delay_ms(1000); // Optional delay
        // Start next conversion
        ADC_Start();
    }

    return 0;
}