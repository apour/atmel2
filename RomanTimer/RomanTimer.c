// main.c

#include <avr/io.h>
#include <util/delay.h> 

/******************************************************************
  Project     : Roman Timer
  It switch relay on PORTB PIN1 for 20s after positive
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
unsigned short temp;

void init()
{ 
    DDRB = 0b00000110;    // PIN0 input, PIN1 output
    state_mode = WaitForStart;
} 

void switchOnRelay()
{
    PORTB|= 0x02;
}

void switchOffRelay()
{
    PORTB&=~0x02;
}

void switchOnLed()
{
    PORTB|= 0x04;
}

void switchOffLed()
{
    PORTB&=~0x04;
}

int main()
{
    // define variables
    init();  

    while(1)
    {
		_delay_ms(100);
        switch (state_mode) 
        {
            case WaitForStart:
                if (PINB & 0x1 == 1)   // positive signal on PIN0
                {
                    state_mode = Starting;
                    // begin starting
                    switchOnRelay();
                    switchOnLed();
                    _timer = 0;
                }
                break;
            case Starting:
                _timer++;
                if (_timer == TIMER_LIMIT)
                {
                    // end starting
                    switchOffRelay();
                    switchOffLed();
                    state_mode = WaitForStop; 
					_timer = 0;
                }
                break;
            case WaitForStop:
				temp = PINB&0x01;			    
                if ((PINB&0x01) == 0) // negative signal on PIN0
                {
                    _timer = 0;
                    state_mode = WaitForStart;
					switchOffLed();
					break;
                }

                if (_timer == 0)
                    switchOffLed();
                else if (_timer == 5)
                    switchOnLed();
                
                _timer++;
                if (_timer == 10)
                {
                    _timer = 0;
                }
                break;
        }
 
    }
}
