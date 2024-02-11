#ifndef _PRJ_H
#define _PRJ_H

#define F_CPU 12000000UL    
#include <avr/io.h> 
#include <avr/sfr_defs.h> 
#include <util/delay.h> 
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "pin_config.h"
#include "adc.h"
#include "uart.h"
#include "timer.h"
#include "state_machine_mode.h"
#include "main.h" 

typedef enum {
    Charge, DisCharge, Measure, Grid
} StateMachineMode;

#endif
