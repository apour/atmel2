// main.c

#include <avr/io.h>

/******************************************************************
  Project     : Roman Timer
  It switch relay on PORT B PIN1 for 20s after positive
  inpuls on PIN0. Next start can be only after negative 
  on PIN0. PIN2 is output for LED status.

  
  MCU         : MEGA8
  F_OSC       : 1,0000 MHz - fuses Lo 0xE1 Hi 0xD9
  
******************************************************************/

#define TIMER_LIMIT     20*10  // 20s -> 20x delay(100ms)

// global variable
typedef enum {
    WaitForStart, Starting, WaitForStop
} StateMachineMode;

StateMachineMode state_mode;
unsigned short _timer;

void init()
{ 
    DDRB = 0x02;    // PIN0 input, PIN1 output
    state_mode = WaitForStart;
} 

int main()
{
    // define variables
    init();  

    while(1)
    {
        switch (state_mode) 
        {
            case WaitForStart:
                if (PORTB & 0x1 == 1)   // positive signal on PIN0
                {
                    state_mode = Starting;
                    // begin starting
                    PORTB|= 0x02;
                    PORTB|= 0x04;
                    _timer = 0;
                }
                break;
            case Starting:
                _timer++;
                if (_timer == TIMER_LIMIT)
                {
                    // end starting
                    PORTB&=~0x02;
                    PORTB&=~0x04;
                    state_mode = WaitForStop; 
                }
                break;
            case WaitForStop:
                if (PINB&0x01 == 0) // negative signal on PIN0
                {
                    _timer = 0;
                    state_mode = WaitForStart;
                }

                if (_timer == 0)
                    PORTB&=~0x04;
                else if (_timer == 5)
                    PORTB|=0x04;
                
                _timer++;
                if (_timer == 10)
                {
                    _timer = 0;
                }
                break;
        }
 
    }
}
