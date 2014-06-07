#include <avr/io.h>
#include <avr/interrupt.h>
#include "attiny25.h"

void init_avr(void) {
	// Init
	CLKPR=(1<<CLKPCE);			// Enable change of prescaler
	CLKPR=0;					// Prescaler: 0, 8Mhz
	TIMSK=0;					// Disable all TIMER0 interrupts
	GIMSK=(1<<INT0);			// Enable external pin interrupt INT0
	TCCR0B=(1<<CS00)|(1<<CS01);	// 8mhz /64 cause 8 bit Timer interrupt every 8us

	// Init counter pins
	GIMSK|=(1<<PCIE);				// Enable Pin Change Interrupt
	PCMSK=(1<<PCINT3)|(1<<PCINT4);	// Enable Pin Change Interrupt for PB3 and PB4
	DDRB &=~((1<<PINB3)|(1<<PINB4));// Set PB3 and PB4 as input
	istat=PINB;

	CounterA=0;
	CounterB=0;
}

ISR(PCINT0_vect) {
	if (((PINB&(1<<PINB3))==0)&&((istat&(1<<PINB3))==(1<<PINB3))) {
		CounterA++;
	}
	if (((PINB&(1<<PINB4))==0)&&((istat&(1<<PINB4))==(1<<PINB4))) {
		CounterB++;
	}
	istat=PINB;
}