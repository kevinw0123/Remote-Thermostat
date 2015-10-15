#include <avr/io.h>
#include <util/delay.h>

void init_lcd() {
  //init_lcd - Do various things to initialize the LCD display
  _delay_ms(15);              // Delay at least 15ms
  writenibble(0x30);          // Use writenibble to send 0011
  _delay_ms(5);               // Delay at least 4msec
  writenibble(0x30);          // Use writenibble to send 0011
  _delay_us(120);             // Delay at least 100usec
  writenibble(0x30);          // Use writenibble to send 0011
  writenibble(0x20);          // Use writenibble to send 0010
  // Function Set: 4-bit interface
  _delay_ms(2);
  writecommand(0x28);
  // Function Set: 4-bit interface, 2 lines
  writecommand(0x0f);         // Display and cursor on
}

void stringout(char *str) {
  //stringout - Print the contents of the character string "str"
  //at the current cursor position.
  unsigned char n = strlen(str);
  unsigned char i=0;
  while (i<n) {
    writedata(str[i]);
    i++;
  }
}

void moveto(unsigned char pos) { 
  //moveto - Move the cursor to the postion "pos"
  writecommand(0x80+pos);
}

void writecommand(unsigned char x) {
  //writecommand - Output a byte to the LCD display instruction register.
  PORTB &= ~(1 << PB0);
  writenibble(x);
  writenibble(x << 4);
  _delay_ms(2);
}

void writedata(unsigned char x) {
  //writedata - Output a byte to the LCD display data register
  PORTB |= (1 << PB0);
  writenibble(x);
  writenibble(x << 4);
  _delay_ms(2);
}

void writenibble(unsigned char x) {
  //writenibble - Output four bits from "x" to the display
  PORTD &= 0x0f;
  PORTD |= (x & 0xf0);
  PORTB &= ~(1 << PB1);
  PORTB |= (1 << PB1);  //add 125ns to the pulse
  PORTB |= (1 << PB1);  
  PORTB &= ~(1 << PB1);
}