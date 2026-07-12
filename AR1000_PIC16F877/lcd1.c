////////////////////////////////////////////////////////////////////////////
////                             LCD.C                                  ////
////                 Driver for common LCD modules                      ////
////                                                                    ////
////  lcd_init()   Must be called before any other function.            ////
////                                                                    ////
////  lcd_putc(c)  Will display c on the next position of the LCD.      ////
////                     The following have special meaning:            ////
////                      \f  Clear display                             ////
////                      \n  Go to start of second line                ////
////                      \b  Move back one position                    ////
////                                                                    ////
////  lcd_gotoxy(x,y) Set write position on LCD (upper left is 1,1)     ////
////                                                                    ////                                                                    ////
////  modified May 8, 2008 to work with the 18F452 by micro222          ////                                                                    ////
////                                                                    ////                                                                    ////
////////////////////////////////////////////////////////////////////////////

// As defined in the following structure the pin connection is as follows:
//
//    PIC  LCD
//    ---  ---
//     D1  enable
//     D0  rs
//     C0  D4
//     C1  D5
//     C2  D6
//     C3  D7
//
//   LCD pins D0-D3 are not used. rw is tied low.

#define lcd_type 2           // 0=5x7, 1=5x10, 2=2 lines
#define lcd_line_two 0x40    // LCD RAM address for the second line

// Pin definition
// C0-C3 map to D4-D7
#define LCD_ENABLE (PIN_D1)
#define LCD_RS (PIN_D0)
#define LCD_D4 (PIN_C0)
#define LCD_D5 (PIN_C1)
#define LCD_D6 (PIN_C2)
#define LCD_D7 (PIN_C3)

//#byte port_c = 0x07

byte CONST LCD_INIT_STRING[4] = { 0x20 | (lcd_type << 2), 0xc, 1, 6 };
                             // These bytes need to be sent to the LCD
                             // to start it up.

void lcd_send_nibble(byte n) {
  // Send data on LCD pin D4-D7
 // port_c = (port_c & 0xf0) | n;

  if(bit_test(n,0)==1) output_high(LCD_D4); else output_low(LCD_D4);
  if(bit_test(n,1)==1) output_high(LCD_D5); else output_low(LCD_D5);
  if(bit_test(n,2)==1) output_high(LCD_D6); else output_low(LCD_D6);
  if(bit_test(n,3)==1) output_high(LCD_D7); else output_low(LCD_D7);

  delay_cycles(1);
  output_high(LCD_ENABLE);
  delay_us(2);
  output_low(LCD_ENABLE);
}

void lcd_send_byte(byte address, byte n) {
  output_low(LCD_RS);
  output_bit(LCD_RS, address);
  delay_cycles(1);
  output_low(LCD_ENABLE);
  lcd_send_nibble(n >> 4);
  lcd_send_nibble(n & 0xf);
}

void lcd_init() {

  byte i;

  output_low(LCD_RS);
  output_low(LCD_ENABLE);
  delay_ms(15);
  for(i = 1; i <= 3; ++i) {
    lcd_send_nibble(3);
    delay_ms(5);
  }
  lcd_send_nibble(2);
  for(i = 0; i <= 3; ++i)
    lcd_send_byte(0, LCD_INIT_STRING[i]);

  delay_ms(5);
}


void
lcd_gotoxy(byte x, byte y) {
   byte address;

   if (y != 1)
     address = lcd_line_two;
   else
     address = 0;
   address += x - 1;
   lcd_send_byte(0, 0x80 | address);
}

void
lcd_putc(char c) {
  switch (c) {
  case '\f':
    lcd_send_byte(0, 1);
    delay_ms(2);
    break;
  case '\n':
    lcd_gotoxy(1, 2);
    break;
  case '\b':
    lcd_send_byte(0, 0x10);
    break;
  default:
    lcd_send_byte(1, c);
    break;
  }
} // lcd_putc

