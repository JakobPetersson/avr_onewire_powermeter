//owdevice - A small 1-Wire emulator for AVR Microcontroller
//
//Copyright (C) 2012  Tobias Mueller mail (at) tobynet.de
//Copyright (C) 2014  Jakob Petersson, kontakt (at) jakobpetersson.se
//
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
// any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//
//VERSION 1.2+ DS2423 for (ATtiny13), ATtiny2313, ATtiny25, ATmega328p


#include <avr/io.h>
#include <avr/interrupt.h>

#if defined(__AVR_ATtiny13A__) || defined(__AVR_ATtiny13__)
#include "attiny13.h"
#elif defined(__AVR_ATtiny25__)
#include "attiny25.h"
#elif defined(__AVR_ATtiny2313A__) || defined(__AVR_ATtiny2313__)
#include "attiny2313.h"
#elif defined(__AVR_ATmega328P__)
#include "atmega328p.h"
#endif

// Calculate and Set your own ID
const uint8_t owid[8]={0x1D, 0xA2, 0xD9, 0x84, 0x00, 0x00, 0x01, 0xD5};

//States / Modes
typedef enum {
	OWM_SLEEP,						// Waiting for next reset pulse
	OWM_RESET,						// Reset pulse received
	OWM_PRESENCE,					// Sending presence pulse
	OWM_READ_COMMAND,				// Read 8 bit of command
	OWM_SEARCH_ROM,					// SEARCH_ROM algorithms
	OWM_MATCH_ROM,					// Test number
	OWM_CHK_RESET,					// Waiting of rising edge from reset pulse
	OWM_GET_ADDRESS,
	OWM_READ_MEMORY_COUNTER,
	OWM_WRITE_SCRATCHPAD,
	OWM_READ_SCRATCHPAD
} onewiremode_t;

//Write a bit after next falling edge from master
//its for sending a zero as soon as possible
#define OWW_WRITE_0 0
#define OWW_WRITE_1 1
#define OWW_NO_WRITE 2

typedef union {
	volatile uint8_t bytes[13];//={1,1,2,0,0,0,0,0,0,0,0,5,5};
	struct {
		uint16_t addr;
		uint8_t read;
		uint32_t counter;
		uint32_t zero;
		uint16_t crc;
	};
} counterpack_t;
counterpack_t counterpack;

volatile uint16_t scrc;			// CRC calculation
volatile uint8_t cbuf;			// Input buffer for a command
volatile uint8_t bitp;			// Pointer to current Byte
volatile uint8_t bytep;			// Pointer to current Bit
volatile onewiremode_t mode;	// Mode
volatile uint8_t wmode;			// If 0 next bit that send the device is 0
volatile uint8_t actbit;		// Current
volatile uint8_t srcount;		// Counter for search rom

PIN_INT {
	uint8_t lwmode=wmode;  //let this variables in registers
	onewiremode_t lmode=mode;
	if (lwmode==OWW_WRITE_0) {
		SET_LOW;
		lwmode=OWW_NO_WRITE;
	} //if necessary set 0-Bit
	DIS_OWINT; //disable interrupt, only in OWM_SLEEP mode it is active
	switch (lmode) {
		case OWM_SLEEP:  
			TCNT_REG=~(OWT_MIN_RESET);
			EN_OWINT; //other edges ?
			break;
		//start of reading with falling edge from master, reading closed in timer isr
		case OWM_MATCH_ROM:  //falling edge wait for receive 
		case OWM_GET_ADDRESS:
		case OWM_READ_COMMAND:
			TCNT_REG=~(OWT_READLINE); //wait a time for reading
			break;
		case OWM_SEARCH_ROM:   //Search algorithm waiting for receive or send
			if (srcount<2) { //this means bit or complement is writing, 
				TCNT_REG=~(OWT_LOWTIME);
			} else 
				TCNT_REG=~(OWT_READLINE);  //init for read answer of master 
			break;	
		case OWM_READ_MEMORY_COUNTER: //a bit is sending 
			TCNT_REG=~(OWT_LOWTIME);
			break;
		case OWM_CHK_RESET:  //rising edge of reset pulse
			SET_FALLING; 
			TCNT_REG=~(OWT_RESET_PRESENCE);  //waiting for sending presence pulse
			lmode=OWM_RESET;
			break;
	}
	EN_TIMER;
	mode=lmode;
	wmode=lwmode;
}


TIMER_INT {
	uint8_t lwmode=wmode; //let this variables in registers
	onewiremode_t lmode=mode;
	uint8_t lbytep=bytep;
	uint8_t lbitp=bitp;
	uint8_t lsrcount=srcount;
	uint8_t lactbit=actbit;
	uint16_t lscrc=scrc;
	//Ask input line sate 
	uint8_t p=((OW_PIN&OW_PINN)==OW_PINN);  
	//Interrupt still active ?
	if (CHK_INT_EN) {
		//maybe reset pulse
		if (p==0) { 
			lmode=OWM_CHK_RESET;  //wait for rising edge
			SET_RISING; 
		}
		DIS_TIMER;
	} else
	switch (lmode) {
		case OWM_RESET:  //Reset pulse and time after is finished, now go in presence state
			lmode=OWM_PRESENCE;
			SET_LOW;
			TCNT_REG=~(OWT_PRESENCE);
			DIS_OWINT;  //No Pin interrupt necessary only wait for presence is done
			break;
		case OWM_PRESENCE:
			RESET_LOW;  //Presence is done now wait for a command
			lmode=OWM_READ_COMMAND;
			cbuf=0;lbitp=1;  //Command buffer have to set zero, only set bits will write in
			break;
		case OWM_READ_COMMAND:
			if (p) {  //Set bit if line high 
				cbuf|=lbitp;
			} 
			lbitp=(lbitp<<1);
			if (!lbitp) { //8-Bits read
				lbitp=1;
				switch (cbuf) {
					case 0x55:lbytep=0;lmode=OWM_MATCH_ROM;break;
					case 0xF0:  //initialize search rom
						lmode=OWM_SEARCH_ROM;
						lsrcount=0;
						lbytep=0;
						lactbit=(owid[lbytep]&lbitp)==lbitp; //set actual bit
						lwmode=lactbit;  //prepare for writing when next falling edge
						break;
					case 0xA5:
						lmode=OWM_GET_ADDRESS; //first the master send an address 
						lbytep=0;lscrc=0x7bc0; //CRC16 of 0xA5
						counterpack.bytes[0]=0;
						break;							
					default: lmode=OWM_SLEEP;  //all other commands do nothing
				}		
			}			
			break;
		case OWM_SEARCH_ROM:
			RESET_LOW;  //Set low also if nothing send (branch takes time and memory)
			lsrcount++;  //next search rom mode
			switch (lsrcount) {
				case 1:lwmode=!lactbit;  //preparation sending complement
					break;
				case 3:
					if (p!=(lactbit==1)) {  //check master bit
						lmode=OWM_SLEEP;  //not the same go sleep
					} else {
						lbitp=(lbitp<<1);  //prepare next bit
						if (lbitp==0) {
							lbitp=1;
							lbytep++;
							if (lbytep>=8) {
								lmode=OWM_SLEEP;  //all bits processed 
								break;
							}
						}				
						lsrcount=0;
						lactbit=(owid[lbytep]&lbitp)==lbitp;
						lwmode=lactbit;
					}		
					break;			
			}
			break;
		case OWM_MATCH_ROM:
			if (p==((owid[lbytep]&lbitp)==lbitp)) {  //Compare with ID Buffer
				lbitp=(lbitp<<1);
				if (!lbitp) {
					lbytep++;
					lbitp=1;
					if (lbytep>=8) {
						lmode=OWM_READ_COMMAND;  //same? get next command
						
						cbuf=0;
						break;			
					}
				} 
			} else {
				lmode=OWM_SLEEP;
			}
			break;
		case OWM_GET_ADDRESS:  
			if (p) { //Get the Address for reading
				counterpack.bytes[lbytep]|=lbitp;
			}  
			//address is part of crc
			if ((lscrc&1)!=p) lscrc=(lscrc>>1)^0xA001; else lscrc >>=1;
			lbitp=(lbitp<<1);
			if (!lbitp) {	
				lbytep++;
				lbitp=1;
				if (lbytep==2) {
					lmode=OWM_READ_MEMORY_COUNTER;
					lactbit=(lbitp&counterpack.bytes[lbytep])==lbitp;
					lwmode=lactbit;
					lsrcount=(counterpack.addr&0xfe0)+0x20-counterpack.addr; 
					//bytes between start and Counter Values, Iam never understanding why so much???
					break;
				} else counterpack.bytes[lbytep]=0;
			}			
			break;	
		case OWM_READ_MEMORY_COUNTER:
			RESET_LOW;
			//CRC16 Calculation
			if ((lscrc&1)!=lactbit) lscrc=(lscrc>>1)^0xA001; else lscrc >>=1;
			p=lactbit;
			lbitp=(lbitp<<1);
			if (!lbitp) {		
				lbytep++;
				lbitp=1;
				if (lbytep==3) {
					lsrcount--;
					if (lsrcount) lbytep--;
					else  {//now copy counter in send buffer
						switch (counterpack.addr&0xFe0) {
						case 0x1E0:
							counterpack.counter=CounterB;
							break;
						case 0x1C0:
							counterpack.counter=CounterA;
							break;
						default: counterpack.counter=0;
						}
					}
				}
				if (lbytep>=13) { //done sending
					lmode=OWM_SLEEP;
					break;			
				}  		 
				if ((lbytep==11)&&(lbitp==1)) { //Send CRC
					counterpack.crc=~lscrc; 
				}			
					 
			}					
			lactbit=(lbitp&counterpack.bytes[lbytep])==lbitp;
			lwmode=lactbit;
			
			break;	
		}
		if (lmode==OWM_SLEEP) {DIS_TIMER;}
		if (lmode!=OWM_PRESENCE)  { 
			TCNT_REG=~(OWT_MIN_RESET-OWT_READLINE);  //OWT_READLINE around OWT_LOWTIME
			EN_OWINT;
		}
		mode=lmode;
		wmode=lwmode;
		bytep=lbytep;
		bitp=lbitp;
		srcount=lsrcount;
		actbit=lactbit;
		scrc=lscrc;
}


int main(void) {
	mode=OWM_SLEEP;
	wmode=OWW_NO_WRITE;
	RESET_LOW;
	
	for(uint8_t i=0;i<sizeof(counterpack);i++) counterpack.bytes[i]=0;

	init_avr();
	SET_FALLING;
	DIS_TIMER;

	sei();
	
	while(1){
		
	}
}