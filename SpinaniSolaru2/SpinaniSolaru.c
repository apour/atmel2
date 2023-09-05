// main.c

#include "prj.h"

/******************************************************************
  Project     : Spínání solaru
  Description : 
  MCU         : MEGA16
  F_OSC       : 1,0000 MHz - fuses Lo 0xE1 Hi 0xD9
  IDE         : AVR Studio 4.18
  Library     : WinAVR-20100110
  Date        : 2021-05-15
  Version     : 
  History     : 

  Note
  This program is distributed in the hope that it will be useful, 
  but WITHOUT ANY WARRANTY; without even the implied warranty of  
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
******************************************************************/

// global variable
StateMachineMode state_mode;
StateMachineMode prev_mode;

void init()
{ 
    uart_init();
    adc_init();
    Timer0_Init();
    DDRB = 0x1F;                       // initialize port B
} 

int main()
{
    // define variables

    init();  
    sei();  // enable interrupt
    while(1)
    {
        // add code here 
 
    }
}
