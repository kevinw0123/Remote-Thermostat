#include <avr/io.h>
#include <string.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "lcd.h"
#include "ds1631.h"

void moduleinit();
char rx_char();
void tx_char(char ch);

volatile unsigned char cstate = 0;		 //declare global variables
volatile int setstate = 0;	 //0 = low, 1 = high
volatile int counter = 0;
volatile int low = 50;
volatile int high = 80;

int main() {
	int oldl = 1000, oldh = 1000, oldt = 1000;//, oldr = 1000;
	unsigned char out[4];
	unsigned char rec[5];
	unsigned char temp[2];
	moduleinit();

	while (1) {
		//receive and convert temperature
		ds1631_temp(temp);
		float c = temp[0];
		if (temp[1]) {
			c += 0.5;
		}
		float f = (9*c)/5;
		int x = f +32;

		//unsigned char rmt = rx_char();

		if (low != oldl) {
			moveto(0x45);	
			stringout("   "); //clear previous number
			moveto(0x45);
			sprintf(out, "%2d", low);
			stringout(out);
			oldl = low;
		}
		if (high != oldh) {
			moveto(0x4d);
			stringout("   ");	//clear previous number
			moveto(0x4d);
			sprintf(out, "%2d", high);
			stringout(out);
			oldh = high;
		}
		if (x != oldt) {
			moveto(5);
			stringout("   ");
			moveto(5);
			itoa(x, out, 10);
			stringout(out);
			oldt = x;
			//send temperature
			if (x > 0) {
				tx_char('+');	
			}
			else if (x < 0) {
				tx_char('-');
			}
			//if 3 digits
			if (out[2] == 0) {
				tx_char('0');
				_delay_ms(5);
				tx_char(out[0]);
				_delay_ms(5);
				tx_char(out[1]);
			}
			//if 2 digits
			else {
				tx_char(out[0]);
				_delay_ms(5);
				tx_char(out[1]);
				_delay_ms(5);
				tx_char(out[2]);
			}
		}

		//receive temperature
		if (UCSR0A & (1 << RXC0)) {
			// character has been received
			if (rx_char() == '+') {
				rec[0] = rx_char();	//100's digit
				if (rec[0] == '0') {		//if 100's digit is 0
					rec[0] = rx_char();
					rec[1] = rx_char();
					rec[2] = 0;
				}
				else {		//else reset rec[0] to 10's digit
					rec[1] = rx_char();
					rec[2] = rx_char();
					rec[3] = 0;
				}
			}
			
			else if (rx_char() == '-') {
				rec[0] = '-';
				rec[1] = rx_char();
				if (rec[1] == '0') {		//if 100's digit is 0
					rec[1] = rx_char();
					rec[2] = rx_char();
					rec[3] = 0;
				}
				else {		//else reset rec[0] to 10's digit
					rec[2] = rx_char();
					rec[3] = rx_char();
					rec[4] = 0;
				}
			}
			moveto(13);
			stringout("   ");
			moveto(13);
			stringout(rec);
		}

		if (x < low) {
			PORTD |= (1 << PD3);
		}
		else {
			PORTD &= ~(1 << PD3);
		}
		if (x > high) {
			PORTD |= (1 << PD2);
		}
		else {
			PORTD &= ~(1 << PD2);
		}
		moveto(0x50);	//move cursor off of screen
	}
	return 0;
}

void moduleinit() {
	DDRD = 0xf0;
	DDRB = 0x03;
	// initialize lcd
	init_lcd();
	writecommand(0x01);

	// leds 					(blue wire)
	DDRD |= (1 << PD2) | (1 << PD3);
	// rotary encoder (yellow wire)
	DDRC &=	~(1 << PC1) | ~(1 << PC2);
	PORTC |= (1 << PC1) |  (1 << PC2);
	// buttons 				(green wire)
	DDRB &= ~(1 << PB3) | ~(1 << PB4);
	PORTB |= (1 << PB3) |  (1 << PB4);
	
	//enable interrupts
	sei();
	PCICR |= (1 << PCIE0) | (1 << PCIE1);	
	PCMSK0 |= (1 << PB3) | (1 << PB4);
	PCMSK1 |= (1 << PC1) | (1 << PC2);
	
	//serial interface
	UBRR0 = 103; // Set baud rate
	UCSR0B |= (1 << TXEN0 | 1 << RXEN0); 	// Enable RX and TX
	UCSR0C = (3 << UCSZ00); 							// Async., no parity,
 	// 1 stop bit, 8 data bits
	DDRC |= (1 << PC3);
	PORTC &= ~(1 << PC3);

	//initialize thermometer
	ds1631_init();
	ds1631_conv();

	// template
	stringout("Temp:");
	moveto(8);
	stringout("Rmt :");
	moveto(0x40);
	stringout("Low :");
	moveto(0x48);
	stringout("High:");
}

ISR(PCINT0_vect) {
	unsigned char button1 = (PINB & (1 << PB3));
	unsigned char button2 = (PINB & (1 << PB4));
	if (!button1) {
		setstate = 1;	//button 1 = hot
	}
	if (!button2) {	//button 2 = cold
		setstate = 0;
	}
}

ISR(PCINT1_vect) {
	unsigned char A = (PINC & (1 << PC1));
 	unsigned char B = (PINC & (1 << PC2));
	if (cstate == 0) {
		if (A) {
			cstate = 1;
			counter++;
		}
		else if (B) {
			cstate = 3;
			counter--;
		}
	}
	else if (cstate == 1) {
		if (A == 0) {
			cstate = 0;
			counter--;
		}
		else if (B) {
			cstate = 2;
			counter++;
		}
	}
	else if (cstate == 2) {
		if (A == 0) {
			cstate = 3;
			counter++;
		}
		else if (B == 0) {
			cstate = 1;
			counter--;
		}
	}
	else {
		if (A) {
			cstate = 2;
			counter--;
		}
		else if (B == 0) {
			cstate = 0;
			counter++;
		}
	}
    
    
	if (counter >= 4) {
		if (setstate) {
			if (high < 999)	{	//max temp
				high++;
			}
		}
		else {
			if (low < 999) {
				low++;
			}
		}
		counter = 0;
	}
	else if (counter <= -4) {
		if (setstate) {
			if (high > -99) {
				high--;
			}
		}
		else {
			if (low > -99) {
				low--;
			}
		}
		counter = 0;
	}
}

char rx_char() {
	// Wait for receive complete flag to go high
	int count = 0;
	while ( !(UCSR0A & (1 << RXC0)) ) {
 		_delay_ms(5);
 		if (count++ > 50) {
 			return 0;
		}
 	}
 	return UDR0;
}

void tx_char(char ch) {
 // Wait for transmitter data register empty
 while ((UCSR0A & (1<<UDRE0)) == 0) {}
 UDR0 = ch;
}