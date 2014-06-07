#include <avr/io.h>
#include <avr/interrupt.h>
#include "attiny13.h"

void init_avr(void) {
	// Init 
	CLKPR=(1<<CLKPCE);			// Enable change of prescaler
	CLKPR=0;					// Prescaler: 0, 9.6Mhz
	TIMSK0=0;					// Disable all TIMER0 interrupts
	GIMSK=(1<<INT0);			// Enable external pin interrupt INT0
	TCCR0B=(1<<CS00)|(1<<CS01);	// 9.6mhz /64 couse 8 bit Timer interrupt every 6,666us

	// Init counter pins
	//PCICR|=(1<<PCIE0);					// Enable Pin Change Interrupts
	//PCMSK0=(1<<PCINT0)|(1<<PCINT1);		// Enable Pin Change Interrupt for PB0 and PB1
	//DDRB &=~((1<<PINB0)|(1<<PINB1));	// Set PB0 and PB1 as input
	//istat=PINB;

	CounterA=0;
	CounterB=0;
}