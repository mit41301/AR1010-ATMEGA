#include 	<avr/io.h>
#include 	<avr/interrupt.h>
#include 	<string.h>
#include 	<stdio.h>
#include 	"mydefs.h"
#include 	<util/delay.h>

#include 	"i2c.h"
#include 	"lcd.h"
#include 	"delay_function.h"

uint16_t  	fre;

#define		step 	5

#define	SLA_W	0x20
#define SLA_R	0x21

uint16_t register_values[18] = {
					0xffff,0x5b15,0xF4B9,0x8012,
					0x0400,0x28aa,0x4400,0x1ee7,
					0x7141,0x007d,0x82ce,0x4f55,
					0x970c,0xb845,0xfc2d,0x8097,
					0x04a1,0xdf6a
};
uint8_t v2[22] = {	0x0,0xC,0xD,0xE,0xF,0xE,0xF,0xE,0xF,0xF,0xF,
					0xF,0xF,0xE,0xF,0xE,0xF,0xE,0xF,0xF,0xF,0xF};
					
uint8_t v1[22] = { 	0xF,0xF,0xF,0xF,0xF,0xE,0xE,0xD,0xD,0xB,0xA,
					0x9,0x7,0x6,0x6,0x5,0x5,0x3,0x3,0x2,0x1,0x0};
//----------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------//
void init_ar1000(uint16_t values[])
{
// This code writes the array values to the ar1000, it is used to calibrate the ar1000
// on power up and it can send the modified array values needed for the seeking tuning etc

	uint8_t addr;
	
	for(addr = 1; addr < 18; addr++)
	{
	
		TWI_start ();
		TWI_send (SLA_W,0);
		TWI_send (addr,0);		//addr
		
		TWI_send ((values[addr] & 0xff00) >>8,0);
		TWI_send ((values[addr] & 0x00ff),0);
		TWI_stop ();
	}
	
	TWI_start ();
	TWI_send (SLA_W,0);
	TWI_send (0,0);				//address 0
	// send R0
	TWI_send ((values[0]&0xff00)>>8,0);
	TWI_send ((values[0]&0x00ff),0);
		
	TWI_stop ();
}
//-------------------------------------------------------------------------------//
void write (uint8_t addr, uint16_t value)
{
	uint8_t value1,value2;

	value1 = (value & 0xff00) >>8;
	value2 = (value & 0x00ff);
	
	TWI_start ();
	TWI_send (SLA_W,0);
	TWI_send (addr,0);		//addr
	// send value
	TWI_send (value1,0);
	TWI_send (value2,1);
	TWI_stop ();
}
//-------------------------------------------------------------------------------//
uint16_t readAR1000 (uint8_t addr)
{
	uint16_t	dat;
	
	TWI_start ();
	TWI_send (SLA_W,0);
	TWI_send (addr,0);		//addr
	
		TWI_start ();
		TWI_send (SLA_R,0);

		dat = TWI_read (0);
		dat <<= 8;
		dat |= TWI_read (1);
		TWI_stop ();
		
	return dat;
}
//-------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------//
void setAR1000 (uint16_t freq)
{
	//unsigned int x = 880;
	//freq = x;
		
	freq -= 690;
	//clear tune bit and chan bits
	register_values[2] &= ~(0x01FF | 0x0200);	
	
	//set chan bits
	register_values[2] =  freq;
	
	//clear seek bit
	register_values[3] &= ~(1 << 14);
	
	//set space = 100k (seek stepping increments in 100k steps)
	register_values[3] |= (1 << 13);
 
	//send the registers to the chip
	write (2,register_values[2]);
	write (3,register_values[3]);
	
	//set tune bit
	register_values[2] |= 0x200;
	write (2,register_values[2]);

}
//---------------------------------------------------------------------------------//
void show_frequency (uint16_t freq)
{
	uint8_t 	display_bit[5];
 
	display_bit[0] = freq / 1000 ;
  
	if ( display_bit[0] == 0 ) 
		display_bit[0] = 0x20;
	else 
		display_bit[0] += 0x30;

	display_bit[1] = (freq / 100)%10 +0x30;
	display_bit[2] = (freq / 10)%10 +0x30;
	display_bit[3] = fre%10+0x30;
	display_bit[4] = 0x30;
  
	LCD_write_english_string(0,2," ");
	LCD_write_char(display_bit[0]);
	LCD_write_char(display_bit[1]);
	LCD_write_char(display_bit[2]);
	LCD_write_english_string(30,2,".");
	LCD_write_char(display_bit[3]);
	LCD_write_char(display_bit[4]);
	LCD_write_english_string(48,2," MHZ");
}
//---------------------------------------------------------------------------------//
// Volume Control
// there are two different fields about volume control in AR1000E
//  Volume   :  D7  ~D10 in register R3
//  Volume2 :  D12~D15 in register R14
//  22 combinations of ( volume2 + volume)  are  recommended.
//---------------------------------------------------------------------------------//
void set_vol (uint8_t vol)
{
	register_values[1]  |= 0x0200;
	write(1, register_values[1]);
	
	register_values[3] = (register_values[3] & ~0x0780) | (v1[vol] << 7); 
	write(3, register_values[3]);
	
	register_values[14] = (register_values[14] & ~0xF000)| (v2[vol] << 12);
	write(14, register_values[14]);
	
	 // clear hmute bit
    register_values[1] &= ~0x0200;
    write(1,register_values[1]);

	
}
//---------------------------------------------------------------------------------//

void init(void)
{
	DDRB = 0XFF;
	PORTB = 0XFF;
  
	DDRD = 0B11100000;
	PORTD = 0XFF;
  
	SPSR |= (1<<SPI2X);           		// 
	SPCR |= (1<<SPE)|(1<<MSTR);     	// 
	
		
	LCD_DDR |= LCD_RST | LCD_DC | LCD_CE | SPI_MOSI | SPI_CLK;	
	
	SFIOR=0x00; 
	
	TWCR = 0x00;
	TWBR = 64;
	

	sei ();
  
	LCD_init(); 
}
//---------------------------------------------------------------------------------//
int main(void)
{

	unsigned int x;
	
	init ();

	fre = 880;			//default 87.50 MHZ
	
	init_ar1000(register_values);
	
	set_vol (20);
	
	setAR1000 (fre);		//100.0 MHZ
		
	LCD_write_english_string(0,0,"FM STEREO");
	//fre = read(2);
	show_frequency(fre);
	LCD_write_inverse_string(0,5," UP ");
	LCD_write_inverse_string(48,5," DOWN ");

	while(1)
	{
		if ((PIND&0x0c) == 0x04)
		{
			delay_nms(500);
			delay_nms(500);
			
			if ( (PIND&0x0c) == 0x04)
			{
				LCD_write_english_string(48,5," DOWN " );
				delay_nms(200);
				
				if(bit_is_clear(PIND, 3))
				{
            
					fre -=step;
					if ( fre <= 875 ) 
						fre = 1080;
          
				setAR1000 (fre);
				
				x = fre;
				//x = readAR1000 (2) + 690;
				
				show_frequency(x);
				delay_nms(200);
				delay_nms(200);
				loop_until_bit_is_set(PIND, 3);
				LCD_write_inverse_string(48,5," DOWN ");
				}

			}
		}	
    
		if ( (PIND&0x0c) == 0x08)
		{
			delay_nms(500);
			delay_nms(500);
			if ( (PIND&0x0c) == 0x08)
			{
          
				LCD_write_english_string(0,5," UP ");
				delay_nms(200);        
               
				fre +=step;
				if ( fre >= 1080) 
				fre = 875;
				setAR1000 (fre);
				
				x = fre;
				//x = readAR1000 (2) + 690;
				show_frequency(x);
				delay_nms(200);
				delay_nms(200);
  
            loop_until_bit_is_set(PIND, 2);
			LCD_write_inverse_string(0,5," UP ");
			}
		}	

	}
}
