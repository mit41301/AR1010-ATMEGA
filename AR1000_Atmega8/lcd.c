#include <avr/io.h>
#include "english_6x8_pixel.h"
#include "delay_function.h"
#include "lcd.h"
#include "delay_function.h"
//---------------------------------------------------------------------------------//
void LCD_init(void)
{
    PORTB &= ~LCD_RST;          // 
    delay_1us();
    PORTB |= LCD_RST;
    
    PORTB &= ~LCD_CE ;			// 
    delay_1us();
    PORTB |= LCD_CE;			// 
    delay_1us();

    LCD_write_byte(0x21, 0);	//
    LCD_write_byte(0xc8, 0);	//
    LCD_write_byte(0x06, 0);	// 
    LCD_write_byte(0x13, 0);	//
    LCD_write_byte(0x20, 0);	// 
    LCD_clear();	        	// 
    LCD_write_byte(0x0c, 0);	// 
        
    PORTB &= ~LCD_CE ;          // 
}
//---------------------------------------------------------------------------------//
void LCD_clear(void)
{
    unsigned int i;

    LCD_write_byte(0x0c, 0);			
    LCD_write_byte(0x80, 0);			

    for (i=0; i<504; i++)
      LCD_write_byte(0, 1);			
}
//---------------------------------------------------------------------------------//
void LCD_set_XY(unsigned char X, unsigned char Y)
{
    LCD_write_byte(0x40 | Y, 0);		// column
    LCD_write_byte(0x80 | X, 0);          	// row
}
//---------------------------------------------------------------------------------//
void LCD_write_char(unsigned char c)
{
    unsigned int line;

    c -= 32;

    for (line=0; line<6; line++)
      LCD_write_byte(font6x8[c][line], 1);
}
//---------------------------------------------------------------------------------//
void LCD_write_inverse_char(unsigned char c)
{
    unsigned int line;

    c -= 32;

    for (line=0; line<6; line++)
      LCD_write_byte(~font6x8[c][line], 1);
}
//---------------------------------------------------------------------------------//
void LCD_write_english_string(unsigned char X,unsigned char Y,char *s)
{
    LCD_set_XY(X,Y);
    while (*s) 
    {
		LCD_write_char(*s);
		s++;
    }
}
//---------------------------------------------------------------------------------//
void LCD_write_inverse_string(unsigned char X,unsigned char Y,char *s)
{
    LCD_set_XY(X,Y);
    while (*s) 
    {
		LCD_write_inverse_char(*s);
		s++;
    }
}
//---------------------------------------------------------------------------------//
void LCD_write_byte(unsigned char data, unsigned char command)
{
    PORTB &= ~LCD_CE ;		        // 
    
    if (command == 0)
      PORTB &= ~LCD_DC ;	        // 
    else
      PORTB |= LCD_DC ;		        // 

    SPDR = data;					//

    while ((SPSR & 0x80) == 0);         // 
	
    PORTB |= LCD_CE ;			//
}
//---------------------------------------------------------------------------------//
void lcd_inverse(void)
{
	LCD_write_byte(0b00001101,0);
}
//---------------------------------------------------------------------------------//
void lcd_no_inverse(void)
{
	LCD_write_byte(0b00001100,0);
}
//---------------------------------------------------------------------------------//
