#include <avr/io.h>
#include <avr/interrupt.h>
#include "atmega328p.h"

// Enables wattmeter
#define _WATT_METER_

void init_avr(void) {
	// Init
	CLKPR=(1<<CLKPCE);			// Enable change of prescaler
	CLKPR=0;					// Prescaler: 0, 8Mhz
	TIMSK0=0;					// Disable all TIMER0 interrupts
	EIMSK=(1<<INT0);			// Enable external pin interrupt INT0
	TCCR0B=(1<<CS00)|(1<<CS01);	// 8mhz /64 cause 8 bit Timer interrupt every 8us

#ifndef _WATT_METER_
	// Init counter pins
	PCICR|=(1<<PCIE0);					// Enable Pin Change Interrupts
	PCMSK0=(1<<PCINT0)|(1<<PCINT1);		// Enable Pin Change Interrupt for PB0 and PB1
	DDRB &=~((1<<PINB0)|(1<<PINB1));	// Set PB0 and PB1 as input
	istat=PINB;
#else
	#define START_TIMER1 TCCR1B=(1<<ICNC1)|(1<<ICES1)|(1<<CS12)|(1<<CS10)
	#define STOP_TIMER1 TCCR1B=(1<<ICNC1)|(1<<ICES1)
	
	// New counter input, uses Input capture on port B0
	START_TIMER1;	// Enable Input Capture Noise Canceler, POS edge detection
					// 8mhz /1024 cause Timer1 count every 128us
					// and (16 bit) Timer1 overflow every 8,388608s (exactly)
	TIMSK1=(1<<ICIE1)|(1<<TOIE1);	// Enable input capture interrupt and Timer1 overflow interrupt
	DDRB &=~(1<<PINB0); 	// Set PB0 as input
	DDRD |=(0<<PIND0); 		// Set PD0 as output (LED)
#endif

	CounterA=0;
	CounterB=0;
}

#ifndef _WATT_METER_
ISR(PCINT0_vect) {
	if (((PINB&(1<<PINB0))==0)&&((istat&(1<<PINB0))==(1<<PINB0))) {
		CounterA++;
	}
	if (((PINB&(1<<PINB1))==0)&&((istat&(1<<PINB1))==(1<<PINB1))) {
		CounterB++;
	}
	istat=PINB;
}
#else
/* TODO: This code is ugly as hell. (Possibly broken) */

volatile uint16_t ov_counter, ov_copy; 	// Overflow counter
volatile uint16_t lastPulse, thisPulse;	// Points for comparison
volatile uint32_t counts;				// 

// Overflow of Timer1
ISR(TIMER1_OVF_vect) {
	ov_counter++;
}

// Count input from electricity-meter.
ISR(TIMER1_CAPT_vect) {
	STOP_TIMER1; 			// 
	thisPulse = ICR1;		// Read Timer1 value
	ov_copy = ov_counter;	// Read Timer1 overflows
	ov_counter = 0;			// Reset Timer1 overflows to zero
	START_TIMER1;			// 

	counts=(uint32_t)thisPulse-(uint32_t)lastPulse+((uint32_t)(ov_copy))*0xFFFF;
	lastPulse=thisPulse;	// Prepare for next time
	
	/* Maximum time between pulses can be
	2^32 * (1024 / 8000000)s = 549755,813888 s = 381,7 days which should be enough.
	
	Every counts (sic) equals exactly 128us of time.

	Calculate watt:
	3600 / Time(s) = 3600 / (0,000128 * counts) = 28125000/counts
	*/
	CounterA = 28125000 / counts;
	CounterB++; 			// Increase Counter0
	PORTD ^= (1<<PIND0);	// Switch LED on/off
}

#endif
