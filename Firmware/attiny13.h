#ifndef ow_slave_DS2423_attiny13_h
#define ow_slave_DS2423_attiny13_h

/*
 * Device: ATtiny13
 * 1-Wire pin - PB1 (Pin 6)
 * CounterA   - ?
 * CounterB   - ?
 *
 */


// 1-Wire pin config
#define OW_PORT PORTB								// 1-Wire port register
#define OW_PORTN (1<<PINB1)							// 1-Wire port number
#define OW_PIN PINB 								// 1-Wire input register
#define OW_PINN (1<<PINB1)							// 1-Wire pin number
#define OW_DDR DDRB									// 1-Wire direction register
//
#define SET_LOW OW_DDR|=OW_PINN; OW_PORT&=~OW_PORTN;// Set 1-Wire line low
#define RESET_LOW OW_DDR&=~OW_PINN;					// Set 1-Wire pin as input

// 1-Wire pin interrupt config
#define EN_OWINT {GIMSK|=(1<<INT0);GIFR|=(1<<INTF0);}	// Enable interrupt 
#define DIS_OWINT  GIMSK&=~(1<<INT0);					// Disable interrupt
#define SET_RISING MCUCR=(1<<ISC01)|(1<<ISC00);			// Set interrupt at rising edge
#define SET_FALLING MCUCR=(1<<ISC01);					// Set interrupt at falling edge
#define CHK_INT_EN (GIMSK&(1<<INT0))==(1<<INT0)			// Test if interrupt enabled
#define PIN_INT ISR(INT0_vect)							// The interrupt service routine

// Timer Interrupt config
#define EN_TIMER TIMSK0 |= (1<<TOIE0); TIFR0|=(1<<TOV0);	// Enable timer interrupt
#define DIS_TIMER TIMSK0 &= ~(1<<TOIE0);					// Disable timer interrupt
#define TCNT_REG TCNT0										// Register of timer-counter
#define TIMER_INT ISR(TIM0_OVF_vect)						// The timer interrupt service routine

// Times
#define OWT_MIN_RESET 51 		// Minimum duration of the Reset impulse
#define OWT_RESET_PRESENCE 4	// Time between rising edge of reset impulse and presence impulse
#define OWT_PRESENCE 20 		// Duration of the presence impulse
#define OWT_READLINE 4			// Duration from master low to read the state of 1-Wire line
				   				// 3 for fast master, 4 for slow master and long lines
#define OWT_LOWTIME 4			// Length of low
								// 3 for fast master, 4 for slow master and long lines

//
volatile uint8_t istat;
volatile uint32_t CounterA;
volatile uint32_t CounterB;

void init_avr(void);

#endif