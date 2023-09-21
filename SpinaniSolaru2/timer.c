// timer.c

#include "prj.h"
#define SIM_MEASURE

#define CHANGE_MACHINE_STATE_INTERVAL   6//16 // 5s
#define CHARGE_INTERVAL                 1 //75 // 75*8s -> 600s
#define DISCHARGE_INTERVAL              1 // 20*8s -> 160s
#define GRID_INTERVAL					1

#define VOLTAGE_LIMIT                   570
#define VOLTAGE_GRID_OFF                507
#define VOLTAGE_GRID_ON					459

#define SETBIT(ADDRESS,BIT) (ADDRESS |= (1<<BIT))
#define CLEARBIT(ADDRESS,BIT) (ADDRESS &= ~(1<<BIT))

volatile unsigned int repeat_cnt0 = 0;
volatile unsigned int voltage_level = 0;

volatile unsigned int state_counter = 0;
volatile unsigned int change_state_interval_counter = 0;

unsigned int sum_voltage = 0;
unsigned char ledFlashMode = 0;
extern StateMachineMode state_mode;
extern StateMachineMode prev_mode;

#ifdef SIM_MEASURE
#define SIM_VOLTAGE_CNT  5
unsigned char sim_voltage_idx = 0;
unsigned int sim_voltage[SIM_VOLTAGE_CNT] = { 450, 550, 580, 560, 440 };			 
#endif 		    			


void uart_endofline()
{
    uart_sendString("\n\r");
}

void switchOffChargeRelay()
{
    uart_sendString("SWITCH OFF CHARGE RELAY");
    uart_endofline();
    CLEARBIT(PORTB, 0);
}

void switchOnChargeRelay()
{
	uart_sendString("SWITCH ON CHARGE RELAY");
    uart_endofline();
    SETBIT(PORTB, 0);
}

void switchOffGridRelay()
{
    uart_sendString("SWITCH OFF GRID RELAY");
    uart_endofline();
    CLEARBIT(PORTB, 4);
}

void switchOnGridRelay()
{
    uart_sendString("SWITCH ON GRID RELAY");
    uart_endofline();
    SETBIT(PORTB, 4);
}

void changeMode(StateMachineMode mode)
{
	prev_mode = mode;
    CLEARBIT(PORTB, 1);
    CLEARBIT(PORTB, 2);
    CLEARBIT(PORTB, 3);
    switch (mode)
    {
        case Charge:
            state_mode = Charge;
            SETBIT(PORTB,1);
			switchOffGridRelay();
			switchOffChargeRelay();
            break;
        case DisCharge:
            state_mode = DisCharge;
            SETBIT(PORTB,2);
			switchOffGridRelay();
			switchOnChargeRelay();
            break;
        case Measure:
            state_mode = Measure;
            SETBIT(PORTB,3);
			switchOffGridRelay();
			switchOnChargeRelay();
            break;
		case Grid:
			state_mode = Grid;
			SETBIT(PORTB, 1);
			switchOnGridRelay();
			switchOffChargeRelay(); // brat taky ze slunicka, kdyz nabijim ze site ?
			break;
        default:
            uart_sendString("M: NEVER GET HERE");
            break;
    }
}

void logStateMode()
{
    char buf[5];
    itoa(change_state_interval_counter, buf, 10);
    uart_sendString(buf);
    uart_send_char('/');
    _delay_ms(1);

    switch (state_mode)
    {
        case Charge:
            itoa(CHARGE_INTERVAL, buf, 10);                       
            uart_sendString(buf);
            uart_sendString("-C: ");
            break;
        case DisCharge:
            itoa(DISCHARGE_INTERVAL, buf, 10);                       
            uart_sendString(buf);
            uart_sendString("-D: ");
            break;
        case Measure:
            uart_sendString("-M: ");
            break;
		case Grid:
			uart_sendString("-G: ");
			break;
        default:
            uart_sendString("M: NEVER GET HERE");
            break;
    }
    _delay_ms(1);
    uart_endofline();
}

void logVoltageLevel(unsigned int value)
{
    char buf[16];
    uart_sendString("V: ");
    itoa(value, buf, 10);
    uart_sendString(buf);
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
            uart_sendString("Stop charge limit - ");
            logVoltageLevel(VOLTAGE_LIMIT);
			uart_sendString("Grid on limit - ");
            logVoltageLevel(VOLTAGE_GRID_ON);
			uart_sendString("Grid off limit - ");
            logVoltageLevel(VOLTAGE_GRID_OFF);
			
            _delay_ms(10);
            sum_voltage>>=4;
#ifdef SIM_MEASURE
			sum_voltage = sim_voltage[sim_voltage_idx];
			sim_voltage_idx++;
			if (sim_voltage_idx == SIM_VOLTAGE_CNT)
				sim_voltage_idx = 0;
#endif 		    			
            uart_sendString("Sum voltage - ");
            logVoltageLevel(sum_voltage);
			
			if (prev_mode == Grid && sum_voltage > VOLTAGE_GRID_OFF)
			{
				changeMode(Charge);
				
				uart_sendString("A1");
				uart_endofline();
			}
			if (sum_voltage < VOLTAGE_GRID_ON)
			{
				changeMode(Grid);
				
				uart_sendString("A2");
				uart_endofline();
			} 
			else if (sum_voltage > VOLTAGE_LIMIT)
            {
            	changeMode(DisCharge);
				
				uart_sendString("A3");
				uart_endofline();
            }
            else
            {
            	changeMode(Charge);
				
				uart_sendString("A4");
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
            uart_sendString("NEVER GET HERE");
            break;
    }
}



// Timer 0  timeout= 1,000 s, fosc = 1,0000 MHz 
//=========================================
void Timer0_Init()
{
    TCCR0 = 0x05; // divider 1024
	TCNT0 = 256 - 244;  
    TIMSK |= _BV(TOIE0); 

    change_state_interval_counter = 0; 
    state_counter = 0;

    changeMode(Measure);
	
    sum_voltage = 8*1024;
    processPossibleChangeState();
}


// Timer 0 interrupt service routine 
// timeout 1,000 s , fosc = 1,0000 MHz 
//====================================
ISR (TIMER0_OVF_vect)
{
	TCNT0 = 256 - 244;  
    if (++repeat_cnt0 == 4) 
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
				modeBit = 1;
				break;
			case DisCharge:
				modeBit = 2;
				break;
			case Measure:
				modeBit = 3;
				break;
			case Grid:
				modeBit = 1;
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
			//switch (ledFlashMode)
			//{
				//case 0:
					//SETBIT(PORTB,modeBit);
					//ledFlashMode++;
					//break;
				//case 1:
					//CLEARBIT(PORTB,modeBit);
					//ledFlashMode++;
					//break;
				//case 5:
					//ledFlashMode = 0;
					//break;
				//default:
					//ledFlashMode++;
					//break;					
			//}
		}		
        logStateMode();
    }
}


