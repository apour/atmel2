// timer.c

#include "prj.h"
#include <stdlib.h>
//#define SIM_MEASURE


#ifdef SIM_MEASURE
#define CHANGE_MACHINE_STATE_INTERVAL   6//16 // 5s
#define CHARGE_INTERVAL                 1 //75 // 75*8s -> 600s
#define DISCHARGE_INTERVAL              1 // 20*8s -> 160s
#define GRID_INTERVAL					1
#else
#define CHANGE_MACHINE_STATE_INTERVAL   16//16 // 5s
#define CHARGE_INTERVAL                 5 //75 // 75*8s -> 600s
#define DISCHARGE_INTERVAL              3 // 20*8s -> 160s
#define GRID_INTERVAL					2
#endif

#define TIMER0_1_SECOND                 256 - 244   // timer for 1S in CPU 12Mhz
#define TIMER0_1_SECOND_REPEAT_CNT      64

// relay pins
#define CHARGE_PIN						0
#define GRID_PIN						4

// fan pin
#define FAN_PIN							5

// LED indicator pins
#define CHARGE_LED_PIN					1
#define DISCHARGE_LED_PIN				2
#define MEASURE_LED_PIN					3

#define VOLTAGE_LIMIT                   565 // 13.2 V
#define VOLTAGE_GRID_OFF                494	// 11.7 V
#define VOLTAGE_GRID_ON					456 // 10.8 V


#define SETBIT(ADDRESS,BIT) (ADDRESS |= (1<<BIT))
#define CLEARBIT(ADDRESS,BIT) (ADDRESS &= ~(1<<BIT))

volatile unsigned int repeat_cnt0 = 0;
volatile unsigned int voltage_level = 0;

volatile unsigned int state_counter = 0;
volatile unsigned int change_state_interval_counter = 0;

unsigned int sum_voltage = 0;
unsigned char ledFlashMode = 0;
StateMachineMode state_mode;
StateMachineMode prev_mode;

#ifdef SIM_MEASURE
#define SIM_VOLTAGE_CNT  5
unsigned char sim_voltage_idx = 0;
unsigned int sim_voltage[SIM_VOLTAGE_CNT] = { 450, 550, 580, 560, 440 };			 
#endif 		    			


void uart_endofline()
{
    uartPutTxt("\n\r");
}

void switchOffFan()
{
    uartPutTxt("SWITCH OFF FAN");
    uart_endofline();
    CLEARBIT(PORTB, FAN_PIN);
}


void switchOnFan()
{
	uartPutTxt("SWITCH ON FAN");
    uart_endofline();
    SETBIT(PORTB, FAN_PIN);
}

void switchOffChargeRelay()
{
    uartPutTxt("SWITCH OFF CHARGE RELAY");
    uart_endofline();
    CLEARBIT(PORTB, CHARGE_PIN);
}

void switchOnChargeRelay()
{
	uartPutTxt("SWITCH ON CHARGE RELAY");
    uart_endofline();
    SETBIT(PORTB, CHARGE_PIN);
}

void switchOffGridRelay()
{
    uartPutTxt("SWITCH OFF GRID RELAY");
    uart_endofline();
    CLEARBIT(PORTB, GRID_PIN);
}

void switchOnGridRelay()
{
    uartPutTxt("SWITCH ON GRID RELAY");
    uart_endofline();
    SETBIT(PORTB, GRID_PIN);
}

void changeMode(StateMachineMode mode)
{
	prev_mode = mode;
    CLEARBIT(PORTB, CHARGE_LED_PIN);
    CLEARBIT(PORTB, DISCHARGE_LED_PIN);
    CLEARBIT(PORTB, MEASURE_LED_PIN);
    switch (mode)
    {
        case Charge:
            state_mode = Charge;
            SETBIT(PORTB, CHARGE_LED_PIN);
			switchOffGridRelay();
			switchOffChargeRelay();
			switchOffFan();
            break;
        case DisCharge:
            state_mode = DisCharge;
            SETBIT(PORTB, DISCHARGE_LED_PIN);
			switchOffGridRelay();
			switchOnChargeRelay();
			switchOnFan();
            break;
        case Measure:
            state_mode = Measure;
            SETBIT(PORTB, MEASURE_LED_PIN);
			switchOffGridRelay();
			switchOnChargeRelay();
			switchOnFan();
            break;
		case Grid:
			state_mode = Grid;
			SETBIT(PORTB, CHARGE_LED_PIN);
			switchOnGridRelay();
			switchOffChargeRelay(); // brat taky ze slunicka, kdyz nabijim ze site ?
			switchOnFan();
			break;
        default:
            uartPutTxt("M: NEVER GET HERE");
            break;
    }
}

void logStateMode()
{
    char buf[5];
    itoa(change_state_interval_counter, buf, 10);
    uartPutTxt(buf);
    uartPutTxt("/");
    _delay_ms(1);

    switch (state_mode)
    {
        case Charge:
            itoa(CHARGE_INTERVAL, buf, 10);                       
            uartPutTxt(buf);
            uartPutTxt("-C: ");
            break;
        case DisCharge:
            itoa(DISCHARGE_INTERVAL, buf, 10);                       
            uartPutTxt(buf);
            uartPutTxt("-D: ");
            break;
        case Measure:
            uartPutTxt("-M: ");
            break;
		case Grid:
			uartPutTxt("-G: ");
			break;
        default:
            uartPutTxt("M: NEVER GET HERE");
            break;
    }
    _delay_ms(1);
    uart_endofline();
}

void logVoltageLevel(unsigned int value)
{
    char buf[16];
    uartPutTxt("V: ");
    itoa(value, buf, 10);
    uartPutTxt(buf);
    uart_endofline();
}

void processPossibleChangeState()
{
    switch (state_mode)
    {
        case Charge:
            change_state_interval_counter++;
            if (change_state_interval_counter == CHARGE_INTERVAL)
            {
				changeMode(Measure);
                sum_voltage = 0;
            }
            break;
        case Measure:
            uartPutTxt("Stop charge limit - ");
            logVoltageLevel(VOLTAGE_LIMIT);
			uartPutTxt("Grid on limit - ");
            logVoltageLevel(VOLTAGE_GRID_ON);
			uartPutTxt("Grid off limit - ");
            logVoltageLevel(VOLTAGE_GRID_OFF);
			
            _delay_ms(10);
            sum_voltage>>=4;
#ifdef SIM_MEASURE
			sum_voltage = sim_voltage[sim_voltage_idx];
			sim_voltage_idx++;
			if (sim_voltage_idx == SIM_VOLTAGE_CNT)
				sim_voltage_idx = 0;
#endif 		    			
            uartPutTxt("Sum voltage - ");
            logVoltageLevel(sum_voltage);
			
			if (prev_mode == Grid && sum_voltage > VOLTAGE_GRID_OFF)
			{
				changeMode(Charge);
				
				uartPutTxt("A1");
				uart_endofline();
			}
			if (sum_voltage < VOLTAGE_GRID_ON)
			{
				changeMode(Grid);
				
				uartPutTxt("A2");
				uart_endofline();
			} 
			else if (sum_voltage > VOLTAGE_LIMIT)
            {
            	changeMode(DisCharge);
				
				uartPutTxt("A3");
				uart_endofline();
            }
            else
            {
            	changeMode(Charge);
				
				uartPutTxt("A4");
				uart_endofline();
            }
            change_state_interval_counter = 0;
            break;
        case DisCharge:
            change_state_interval_counter++;
            if (change_state_interval_counter == DISCHARGE_INTERVAL)
            {
				changeMode(Measure);
                sum_voltage = 0;
            }
            break;
		case Grid:
			change_state_interval_counter++;
			if (change_state_interval_counter == GRID_INTERVAL)
			{
				changeMode(Measure);
				sum_voltage = 0;
			}
			break;				
        default:
            uartPutTxt("NEVER GET HERE");
            break;
    }
}



// Timer 0  timeout= 1,000 s, fosc = 12,0000 MHz 
//=========================================
void Timer0_Init()
{
    TCCR0 = 0x05; // divider 1024
	TCNT0 = TIMER0_1_SECOND;  
    TIMSK |= _BV(TOIE0); 

    change_state_interval_counter = 0; 
    state_counter = 0;

    changeMode(Measure);
	
    sum_voltage = 8*1024;
    processPossibleChangeState();
}


// Timer 0 interrupt service routine 
// timeout 1,000 s , fosc = 12,0000 MHz 
//====================================
ISR (TIMER0_OVF_vect)
{
	TCNT0 = TIMER0_1_SECOND;  
    // test for 1 S
    // if (++repeat_cnt0 == TIMER0_1_SECOND_REPEAT_CNT) 
    //     {
    //     repeat_cnt0 = 0; 
    //     uartPutTxt("A");
    //     uart_flush();
    //     }
    if (++repeat_cnt0 == TIMER0_1_SECOND_REPEAT_CNT) 
    {
        repeat_cnt0 = 0; 
        // write your code 
        
        state_counter++;
        if (state_mode == Measure)
        {
            // read voltage on ADC0
            voltage_level = adc_get_value(0); 
            logVoltageLevel(voltage_level);
            sum_voltage+= voltage_level;
        }

        if (state_counter == CHANGE_MACHINE_STATE_INTERVAL)        
        {
            state_counter = 0;
            processPossibleChangeState();
        }

		// flash
		char modeBit = 0;
		switch (state_mode)
		{
			case Charge:
				modeBit = CHARGE_LED_PIN;
				break;
			case DisCharge:
				modeBit = DISCHARGE_LED_PIN;
				break;
			case Measure:
				modeBit = MEASURE_LED_PIN;
				break;
			case Grid:
				modeBit = CHARGE_LED_PIN;
				break;
		}
		
		if (state_mode == Grid)
		{
			SETBIT(PORTB, modeBit);	
		}
		else
		{
			if (ledFlashMode == 0)
			{
				SETBIT(PORTB, modeBit);
				ledFlashMode = 1;
			}
			else
			{
				CLEARBIT(PORTB, modeBit);
				ledFlashMode = 0;
			}
		}		
        logStateMode();
    }
}


