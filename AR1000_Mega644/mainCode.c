/*
    	5-5-2010
    	Modified from Spark Fun Electronics© 2009
    	Nathan Seidle, spark at sparkfun.com
    	Peter Loy, pel26
    	Stephanie Pancoast, slp56
    	ECE 4760
	
	LCD Code Written by Ruibing Wang (rw98@cornell.edu)
 	Mods for 644 by brl4@cornell.edu

	SETUP: LCD
	[LCD] -	[Mega644 Pin]
    	1 GND -	GND
    	2 VCC - +5V
 	3 VLC 10k trimpot wiper (trimpot ends go to +5 and gnd) 
 	4 RS -	PA0
 	5 RD -	PA1
 	6 EN -	PA2
 	11 D4 -	PA4
 	12 D5 -	PA5
 	13 D6 -	PA6
	14 D7 -	PA7 

	SETUP: AR1010
	[AR1010] - [Mega644]
	1 VCC - +3.3V
	2 GND - GND
	3 LOUT - Audio Jack Output L
	4 ROUT - Audio Jack Output R
	5 CS - GND
	6 CLK - PC0
	7 DATA - PC1

	SETUP: Keyscanner
	assumes a standard 4x4 keypad connected to a port (PORTD)

    Example Interface to AR1010
	Using ATmega644 at 16MHz
*/ 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/eeprom.h> 
#include "lcd_lib.h"
#include "i2c.h"

#define FOSC 16000000
#define BAUD 9600
#define MYUBRR 103

#define sbi(port, bit_mask)   ((port) |= (uint8_t)(1 << bit_mask))
#define cbi(port, bit_mask)   ((port) &= (uint8_t)~(1 << bit_mask))

#define AR1000_W 0x20 //Write address of AR1000
#define AR1000_R 0x21//read address

#define ADDR_STATUS	0x13 // the address of status register
#define MASK_STC (1<<5) //0x0020  // Seek/Tune/PowerOn complete  D5 in  adress 13H 
#define MASK_SF	 (1<<4) //0x0010  //  Seek Fail D4 in address 13H
#define MASK_ST	 (1<<3) //0x0008  //  Stereo  D3 in address 13H
#define MASK_READCHAN 0xFF80 // D7~D15 in address 13H
#define SHIFT_READCHAN 7

#define AR1000_MUTE_ON  { uint16_t temp = ar1000_read(1); ar1000_write(1, temp | (1<<1)); } //Reg_Data[1].BIT.B1 = ON;
#define AR1000_MUTE_OFF { uint16_t temp = ar1000_read(1); ar1000_write(1, temp & ~(1<<1)); } //Reg_Data[1].BIT.B1 = OFF;}

#define AR1000_TUNE_ON 	{ uint16_t temp = ar1000_read(2); ar1000_write(2, temp | (1<<9)); } // Reg_Data[2].BIT.B9 = ON;
#define AR1000_TUNE_OFF { uint16_t temp = ar1000_read(2); ar1000_write(2, temp & ~(1<<9)); } //Reg_Data[2].BIT.B9 = OFF;

#define AR1000_SEEK_ON 	{ uint16_t temp = ar1000_read(3); ar1000_write(3, temp | (1<<14)); } //Reg_Data[3].BIT.B14 = ON;}
#define AR1000_SEEK_OFF { uint16_t temp = ar1000_read(3); ar1000_write(3, temp & ~(1<<14)); } //Reg_Data[3].BIT.B14 = OFF;}

// Keyscan
#define maxkeys 16
#define PORTDIR DDRD
#define PORTDATA PORTD
#define PORTIN PIND

//State machine state names
#define NoPush 1 
#define MaybePush 2
#define Pushed 3
#define StillPush 4
#define MaybeNoPush 5

const int8_t LCD_initialize[] PROGMEM = "LCD Initialized \0";
const int8_t LCD_station[] PROGMEM =    "Currently on... \0";
const int8_t LCD_seeking[] PROGMEM =    "Still looking...\0" ;
const int8_t LCD_set[] PROGMEM =        "Set favorite to \0" ;
const int8_t LCD_doneSet[] PROGMEM =    "Favorite is set \0" ;
const int8_t LCD_incTH[] PROGMEM = 	"Scan Thresh +   \0" ;
const int8_t LCD_decTH[] PROGMEM =      "Scan Thresh -   \0" ;
const int8_t LCD_resetTH[] PROGMEM =    "Reset Thresh   \0" ;
const int8_t LCD_error[] PROGMEM =      "Uh oh           \0" ; //error
//Define functions
//======================
void i2c_SendStart(void);
void i2c_SendStop(void);
void i2c_WaitForComplete(void);
unsigned char i2c_SendByte(unsigned char data);
unsigned char i2c_ReceiveByte(unsigned char ackFlag);

void ioinit(void);
void blinkLed(char num);
void togled(char num);

uint16_t ar1000_read(uint8_t address);
void ar1000_write(char reg_address, uint16_t reg_value);
void ar1000_readall(void);
void ar1000_write_array(void);
void ar1000_init(void);
void ar1000_setvolume(uint8_t volume_level);
void ar1000_tuneto(uint16_t freq_kHz);
void ar1000_tune_hilo(uint16_t freq_kHz);
void ar1000_seek(char updown);
uint16_t getChannel(void);
uint16_t ar1000_rssi(void);
void changeThresh(char upDown);

void memInit(void); 
void setStationMem(uint16_t freq, char num);
uint16_t getStationMem(char num);

void lcd_init(void);
void lcdNewMsg(uint16_t freq);

void keyscan(void);
void init_key(void);


//RSSI 49-54 is pretty good. 34-41 is complete static

//Global Variables
//======================
char led, setMemFlag, scanThresh;
uint16_t currentChannel;
uint16_t stationMem[3];


//Register conaining default values for the AR1000, these are the default values from the programming guide.
uint16_t register_values[18] = 
{	
	
	0xFFFF, //R0
	0x5B15, //R1
	0xF4B9, //R2 Tune/Channel
	0x8012, //R3 seekTHD = 18
	0x0400, //R4
	0x28AA, //R5
	0x4400, //R6
	0x1EE7,	// R7
	0x7141,	// R8
	0x007D,	// R9
	0x82C6,	// R10  disable wrap
	0x4F55,	// R11. <--- (disable xo_output)
	0x970C,	// R12.
	0xB845,	// R13
	0xFC2D,	// R14 : Volume control 2
	0x8097,	// R15
	0x04A1,	// R16
	0xDF6A	// R17
};

// volume control (increasing)
unsigned char AR1000vol[22] =
{ 
	0x0F,	// step 0
	0xCF,	// 1
	0xDF,	// 2
	0xEF,	// 3
	0xFF,	// 4
	0xEE,	// 5
	0xFE,	// 6
	0xED,	// 7
	0xFD,	// 8
	0xFB,	// 9
	0xFA,	// 10
	0xF9,	// 11
	0xF7,	// 12
	0xE6,	// 13
	0xF6,	// 14
	0xE5,	// 15
	0xF5,	// 16
	0xE3,	// 17
	0xF3,	// 18
	0xF2,	// 19
	0xF1,	// 20
	0xF0	// 21 <------ default setting
};

int8_t lcd_buffer[35];        // LCD display buffer

unsigned char PushState;	//state machine

// The raw keyscan
unsigned char key ;   
// The decoded button number
unsigned char butnum ;
// For debounce checking
unsigned char maybe;

//key pad scan table
unsigned char keytbl[16]={0xee, 0xed, 0xeb, 0xe7, 
						  0xde, 0xdd, 0xdb, 0xd7, 
						  0xbe, 0xbd, 0xbb, 0xb7, 
						  0x7e, 0x7d, 0x7b, 0x77};


//======================

int main (void)
{

	ioinit(); //Setup IO pins and defaults
	lcd_init();
	init_key();
	mem_init();

	ar1000_tuneto(935);

	return(0);
}

uint16_t ar1000_rssi(void)
{
#define ADDR_RSSI	0x12 
#define MASK_RSSI 	0xFE00
#define SHIFT_RSSI  9

	uint16_t rssi;
	
	rssi = ar1000_read(ADDR_RSSI);
	rssi &= MASK_RSSI;
	rssi >>= 9;

	return(rssi);
}

// Volume Control- NOT USED
// There are two different fields about volume control in AR1000F
//  Volume : D7~D10 in register R3
//  Volume2 : D12~D15 in register R14
//  22 combinations of (volume2 + volume) are recommended.
void ar1000_setvolume(uint8_t volume_level)
{
	uint16_t reg3, reg14;
	
	reg3 = ar1000_read(3) & 0xF87F; //Zero out bits D7-D10
	reg3 |= ( (AR1000vol[volume_level] & 0x0F) << 7); //Mask in D7-D10
	
	reg14 = ar1000_read(14) & 0x0FFF; //Zero out bits D12-D15
	reg14 |= ((AR1000vol[volume_level] & 0xF0) << 8); //Mask in D12-D15
	
	ar1000_write(3, reg3);
	ar1000_write(14, reg14);
}

//Tunes the AR1000 to a given station.
//Calculate AR1000 CHAN id  :  Freq (MHz) = 69 + 0.1*CHAN
//Example, sending 973 will tune to 97.3MHz
void ar1000_tuneto(uint16_t freq_kHz)
{
	uint16_t channel, temp;

	/*
	1) Set hmute Bit 
	2) Clear TUNE Bit 
	3) Clear SEEK Bit
	4) Set BAND/SPACE/CHAN Bits
	5) Enable TUNE Bit 
	6) Wait STC flag (Seek/Tune Comlete, in Status Register
	7) Clear hmute Bit 
	8) Update Functions (optional) 
	*/

	//Clear tune bit
	AR1000_TUNE_OFF;

	//Set Channel
	channel = freq_kHz - 690;
	temp = ar1000_read(2); //Read 
	temp &= 0xFE00; //Mask
	temp |= channel;
	ar1000_write(2, temp); //Write
	
	//Enable tune bit
	AR1000_TUNE_ON;
	
	//Wait for tune to stabilize (STC flag)
	temp = 0;
	while(temp == 0)
	{
		temp = ar1000_read(ADDR_STATUS) & MASK_STC;
	}

	currentChannel = freq_kHz;

	lcdNewMsg(freq_kHz);
	
}

//This is some weird function in AR1000 example code provided by Airoha
//Looks like it takes the RSSI into account and then fine tunes the station
//I can't hear much of a difference, but it looks fancy.
void ar1000_tune_hilo(uint16_t freq_kHz)
{
	uint16_t temp;

	AR1000_MUTE_ON; //Set mute ON before TUNE
	AR1000_SEEK_OFF; //Clear seek

	//Read Low-Side LO Injection
	//R11 --> clear  D15,  clear D0/D2,  D3 is the same as default
	temp = ar1000_read(11) & 0x7FFA;
	ar1000_write(11, temp);

	//TUNE to FreqKHz with current setting
	ar1000_tuneto(freq_kHz); //This function turns on TUNE and waits for STC flag
	//Low-side TUNE Ends 

	uint16_t status = ar1000_read(ADDR_RSSI);
	uint16_t rssi = (status & MASK_RSSI);

	//Read Hi-Side LO Injection
	// R11-->set D15, set D0/D2,  D3 is the same as default
	temp = ar1000_read(11) | 0x8005;
	ar1000_write(11, temp);
	
	//TUNE to FreqKHz with current setting
	ar1000_tuneto(freq_kHz); //This function turns on TUNE and waits for STC flag
	//High-side TUNE Ends 

	status = ar1000_read(ADDR_RSSI);
	rssi = rssi - (status & MASK_RSSI);	
	if (rssi < 0) //errata in 0.82
	{ 	
		// LO
		// R11--> clear D15, set D0/D2, D3 is the same as default
		temp = (ar1000_read(11) & 0x7FFF) | 0x0005;
		ar1000_write(11, temp);
	}
	else
	{ 
		//HI
		//R11-->  set D15, clear D0/D2, D3 is the same as default
		temp = (ar1000_read(11) | 0x8000) & 0xFFFA;
		ar1000_write(11, temp);
	}

	//Fine-tune!!
	//TUNE to FreqKHz with current setting
	ar1000_tuneto(freq_kHz); //This function turns on TUNE and waits for STC flag

	AR1000_MUTE_OFF;

}

//Starts scanning the stations for a minimum set threshold. I found the bit to enable
//wrapping so the every time the function is called, it searches up, and wrap back to 88MHz
//if it doesn't find a good station. 1- seek up, 0- seek down
void ar1000_seek(char updown)
{
	#define ADDR_SEEK_SETTING	0x11
	#define SEEK_SETTING	0x2000
	#define SEEK_MASK		0xC3FF
	#define SEEK_TH_MASK	0xFF80
	//#define SEEK_TH			20 //A higher threshold causes stronger stations to be found	

	uint16_t temp;
	
	char space = 1; //0.1MHz scanning
	char band = 0; //US/Europe radio band
	
	AR1000_MUTE_ON;
	AR1000_TUNE_OFF;
	AR1000_SEEK_OFF;
	
	//Enable wrap during seek - I found bit D3 enables wrap, by trial and error. Seems to work
	temp = ar1000_read(10) | (1<<3); //0x82C6 = 1000 0010 1100 0110
	ar1000_write(10, temp);
	
	//Setting before seek
	temp = (ar1000_read(17) & SEEK_MASK) | SEEK_SETTING;
	ar1000_write(17, temp);

	AR1000_SEEK_ON;
	temp = ar1000_read(3);
	if(space == 1)	temp |= (1<<13); //Set space
	if(updown == 1)	temp |= (1<<15); //Set seek up or down
	else temp &= 0b10111111111111111;//(0<<15);
	temp = (temp & 0xE7FF) | band; //Set Band
	temp &= SEEK_TH_MASK; //Clear out the seek threshold
	temp |= scanThresh; //Set threshold
	ar1000_write(3, temp);
	
	//Wait for tune to stabilize (STC flag), meanwhile display we're seeking
	CopyStringtoLCD(LCD_seeking,0,0);
	temp = 0;
	while(temp == 0)
	{
		temp = ar1000_read(ADDR_STATUS) & MASK_STC;
	}
	blinkLed(0xAA);
	temp = ar1000_read(ADDR_STATUS) & MASK_SF;
	if(temp != 0)
	{
		CopyStringtoLCD(LCD_station,0,0);
		return;
	}
	blinkLed(0x55);
	temp = ar1000_read(ADDR_STATUS) & MASK_READCHAN;
	uint16_t freq_kHz = 690 + (temp >> SHIFT_READCHAN); //Determine what channel we found

	//Restore setting after seek
	ar1000_write(17, register_values[17]);

	//Fine-tune with auto hilo rejection
	ar1000_tune_hilo(freq_kHz);

	AR1000_MUTE_OFF;     
}

//Change threshold for scanning stations
//upDown=1, increase threshold by 1, upDown=0, decrease threshold by 1, upDown=2, reset to initial value (25)
void changeThresh(char upDown)
{
	if (upDown==1)
	{
		if (scanThresh==0b01111111){
			return;
		}
		scanThresh++;
		CopyStringtoLCD(LCD_incTH,0,0);
		_delay_ms(500);
		CopyStringtoLCD(LCD_station,0,0);
	}
	else if (upDown==0)
	{
		if (scanThresh==0b00000001){
			return;
		}
		scanThresh--;
		CopyStringtoLCD(LCD_decTH,0,0);
		_delay_ms(500);
		CopyStringtoLCD(LCD_station,0,0);
	}
	else // reset
	{
		scanThresh = 25;
		CopyStringtoLCD(LCD_resetTH,0,0);
		_delay_ms(500);
		CopyStringtoLCD(LCD_station,0,0);
	}
}

//Reads a memory register from the AR1000
uint16_t ar1000_read(uint8_t address_to_read)
{
	char byte1 = 0, byte2 = 0;
	char ack;
	
AGAIN:
	i2c_SendStart(); //Send start condition 
	ack = i2c_SendByte(AR1000_W);	 //Send slave device address with write
	ack &= i2c_SendByte(address_to_read);	//Send address to read
	if(ack == 0)
	{
		goto AGAIN;
	}
	
	
	i2c_SendStart(); //Send start condition 
	i2c_SendByte(AR1000_R);	 //Ask device to read the value at the requested address
	
	if(inb(TWSR) == TW_MR_SLA_ACK)
	{
		byte1 = i2c_ReceiveByte(TRUE);
		byte2 = i2c_ReceiveByte(TRUE);
	}
	else
	{
		// device did not ACK it's address,
		// data will not be transferred
		// return error
		//retval = I2C_ERROR_NODEV;
		blinkLed(0x01);
	}

	i2c_SendStop();
	
	//Combine two bytes into one 16-bit word
	int16_t temp = byte1 << 8;	
	temp |= byte2;
	
	return(temp);
}

void ar1000_write(char reg_address, uint16_t reg_value)
{
	char ack;
	uint8_t value1 = (reg_value & 0xFF00) >> 8;
	uint8_t value2 = (reg_value & 0x00FF);

AGAIN:
	i2c_SendStart(); //Send start condition 
	ack = i2c_SendByte(AR1000_W);	
	ack &= i2c_SendByte(reg_address); //Send address to write to
	ack &= i2c_SendByte(value1); //Send the two data bytes to be stored
	ack &= i2c_SendByte(value2);
	i2c_SendStop();

	if(ack == 0)
	{;
		goto AGAIN;
	}
}

void ar1000_write_array(void)
{
	// This code writes the array values to the ar1000, it is used to calibrate the ar1000
	// on power up and it can send the modified array values needed for the seeking tuning etc
	
	//The example AR1000 code disables the analog and digital blocks
	// then write to the 0x01 to 0x11 registers
	// then enables the analog and digital blocks - so that's what we will do as well

	//Write the first register
	ar1000_write(0, register_values[0] & 0xFFFE); //<--- Notice we force the enable bit to zero

	for(int i = 1 ; i < 18 ; i++)
		ar1000_write(i, register_values[i]); //Write registers 1 to 17 to AR1000
	
	ar1000_write(0, register_values[0]); //Re-write the first register, this will set the enable bit
}

//Reads and prints all 16 registers (16-bits wide) of the AR1000
void ar1000_readall(void)
{
	uint16_t x, register_value;
	for(x = 0 ; x < 0x1D ; x++)
	{
		register_value = ar1000_read(x);
	}
}

void ar1000_init(void)
{
	_delay_ms(100); //Wait for power to stabilize
	scanThresh = 25;
	
	ar1000_write_array(); //Init the AR1000 by writing the initial recommended values

	uint16_t status = 0;
	while(status == 0)
	{
		status = ar1000_read(ADDR_STATUS);

		status = status & MASK_STC;
		_delay_ms(10);
	}
}

// So LCD can tell what channel we're actually on
uint16_t getChannel(void)
{
	uint16_t status;
	
	status = ar1000_read(ADDR_STATUS);
	return (status & MASK_READCHAN);
}

//Setup IO pins
void ioinit (void)
{
	
	//1 = output, 0 = input
    	DDRC = 0b11111111; //All outputs

    	// For debugging with LED
    	DDRB = 0xff;
    	led = 0xff;
    	PORTB = led;

	//initialize I2C hardware
	TWCR = 0x00;
	TWBR = 64;

	cbi(TWCR, TWEA);
	sbi(TWCR, TWEN);
	
	ar1000_init(); //Initialize AR1000
}

void blinkLed(char num){
	_delay_ms(500);
	led = led^num;
	PORTB = led;
	_delay_ms(500);
	led = led^num;
	PORTB = led;
}

void togled(char num){
	led = led^num;
	PORTB = led;
}


//==========================
//
//Favorite station functions
//
//==========================
void mem_init(void){
	uint16_t chanSet = eeprom_read_byte((uint8_t*)1); 
	if (chanSet!=0xA) // see if we've previously set channel. A is arbutirarily schosen
	{
		eeprom_write_byte((uint8_t*)(1),0xA);// now channels are set

		// Put 88.0 in all locations until set otherwise
		eeprom_write_word((uint16_t*)(2),880);
		eeprom_write_word((uint16_t*)(4),880);
		eeprom_write_word((uint16_t*)(6),880);
	}

	setMemFlag=0;
}

void setStationMem(uint16_t freq, char num){
	if (num>2){
		CopyStringtoLCD(LCD_error,0,0);
		return;
	}
	eeprom_write_word((uint16_t*)((num+1)<<1),freq); // we've stored some value

	setMemFlag=0;
}

uint16_t getStationMem(char num){
	if (num>2){
		CopyStringtoLCD(LCD_error,0,0);
	}
	return eeprom_read_word((uint16_t*)((num+1)<<1));
}


//==========================
//
//LCD functions
//
//==========================
void lcd_init(void) 
{
	LCDinit();	//initialize the display
	LCDcursorOFF();
	LCDclr();				//clear the display
	LCDGotoXY(0,0);
	CopyStringtoLCD(LCD_initialize, 0, 0);
}

void lcdNewMsg(uint16_t freq)
{
	float freqf = (float)(freq)/10;
	sprintf(lcd_buffer, "%-.1f     ", (float)freqf);
	CopyStringtoLCD(LCD_station,0,0);
	LCDGotoXY(0,1);
	LCDstring(lcd_buffer, strlen(lcd_buffer));

}

//==========================
//
//Keypad functions
//
//==========================
void init_key(void){     
	//init the state machine
	PushState = NoPush;
}
void keyscan(void)
{
//keyscan loop
  while(1)
  { 
	//get lower nibble
  	PORTDIR = 0x0f;
  	PORTDATA = 0xf0; 
  	_delay_us(5);
  	key = PORTIN;
  	  
  	//get upper nibble
  	PORTDIR = 0xf0;
  	PORTDATA = 0x0f; 
  	_delay_us(5);
  	key = key | PORTIN;
  	  
  	//find matching keycode in keytbl
  	if (key != 0xff)
  	{   
  	  for (butnum=0; butnum<maxkeys; butnum++)
  	  {   
  	  	if (keytbl[butnum]==key)  break;   
  	  }

  	  if (butnum==maxkeys) butnum=0;
  	  else butnum++;	   //adjust by one to make range 1-16
  	} 
  	else butnum=0;
  	 
  	//PORTB = ~butnum ; from original program: led displays key number pressed

	//Debounce keypress
	switch (PushState)
	{
		case NoPush: //***DONE***
			if (butnum == 0)
			{
				PushState = NoPush;
			}
			else
			{
				PushState = MaybePush;
				maybe = butnum;
			}
			break;

		case MaybePush: //***DONE***
			if (butnum == maybe)
			{
				PushState = Pushed;
			}
			else
			{
				PushState = NoPush;
				maybe = 0;
			}
			break;

		case Pushed:
			// Set up seperate tasks for the different keys (1 task for each key) and detect which key was pressed
			/*
			keys and their corresponding functions (first number is butnum designation, second is keypad representation:
			1(1):Seek Up
			2(2):Tune Up
			3(3):Increase Threshold
			4(A):
			5(4):Seek Down
			6(5):Tune Down
			7(6):Decrease Threshold
			8(B):
			9(7):
			10(8):
			11(9): Reset Threshold
			12(C):
			13(0): Favorite Channel 1 Play/Set
			14(F): Favorite Channel 2 Play/Set
			15(E): Favorite Channel 3 Play/Set
			16(D): Set Favorite Channel

			*/
			if (maybe == 1)
			{
				ar1000_seek(1);
			}
			else if (maybe == 2)
			{
				ar1000_tuneto(++currentChannel);
			}
			else if (maybe == 3)
			{
				changeThresh(1);
			}
			else if (maybe == 4)
			{
				setMemFlag = 1;
			}
			else if (maybe == 5)
			{
				ar1000_seek(0);
			}
			else if (maybe == 6)
			{
				ar1000_tuneto(--currentChannel);
			}
			else if (maybe == 7)
			{
				changeThresh(0);
			}
			else if (maybe == 8)
			{
			}
			else if (maybe == 9)
			{
			}
			else if (maybe == 10)
			{
			}
			else if (maybe == 11)
			{
				changeThresh(2); // reset to initial
			}
			else if (maybe == 12)
			{
			}
			else if (maybe == 13)
			{
				if (setMemFlag)
				{
					setStationMem(currentChannel,0);
					CopyStringtoLCD(LCD_doneSet,0,0);
					_delay_ms(1000);
					CopyStringtoLCD(LCD_station,0,0);
				}
				else
				{
					ar1000_tuneto(getStationMem(0));
				}
			}
			else if (maybe == 14)
			{
				if (setMemFlag)
				{
					setStationMem(currentChannel,1);
					CopyStringtoLCD(LCD_doneSet,0,0);
					_delay_ms(1000);
					CopyStringtoLCD(LCD_station,0,0);
				}
				else
				{
					ar1000_tuneto(getStationMem(1));
				}

			}
			else if (maybe == 15)
			{
				if (setMemFlag)
				{
					setStationMem(currentChannel,2);
					CopyStringtoLCD(LCD_doneSet,0,0);
					_delay_ms(1000);
					CopyStringtoLCD(LCD_station,0,0);
				}
				else
				{
					ar1000_tuneto(getStationMem(2));
				}
			}
			else if (maybe == 16)
			{
				if (setMemFlag) 
				{
					CopyStringtoLCD(LCD_station,0,0);
				}
				else {
					CopyStringtoLCD(LCD_set,0,0);
				}
				setMemFlag=~setMemFlag;
				
			}


			if (butnum == maybe)
			{
				PushState = StillPush;
			}
			else
			{
				PushState = MaybeNoPush;
			}

			break;

		case StillPush: //***DONE***
			if (butnum == maybe)
			{
				PushState = StillPush;
			}
			else
			{
				PushState = MaybeNoPush;
			}
			break;

		case MaybeNoPush: //***DONE***
			if (butnum == maybe)
			{
				PushState = StillPush;
			}
			else
			{
				PushState = NoPush;
				maybe = 0;
			}
			break;
	}
  	
  } // end while
}   //end keyscan


//==========================
//
//I2C functions
//
//==========================

void i2c_SendStart(void)
{
	// send start condition
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

	i2c_WaitForComplete();
}

void i2c_SendStop(void)
{
	// transmit stop condition
	TWCR = (1<<TWSTO);
}

void i2c_WaitForComplete(void)
{
	// wait for any previous i2c stuff to complete before sending new data
	while (!(TWCR & (1<<TWINT)));
}

unsigned char i2c_SendByte(unsigned char data)
{
	// save data to the TWDR
	TWDR = data;
	// begin send
	TWCR = (1<<TWINT)|(1<<TWEN);

	// wait until transmission completed
	while(!(TWCR & (1<<TWINT)));

	// check value of TWI Status Register. Mask prescaler bits
	uint8_t twst = TWSR & 0xF8;
	
	if( twst == 0x18) return 1; //SKA+W was tranmitted, ACK received
	if( twst == 0x28) return 1; //Data was tranmitted, ACK received
	return 0;
}

unsigned char i2c_ReceiveByte(unsigned char ackFlag)
{
	// begin receive over i2c
	if( ackFlag )
	{
		// ackFlag = TRUE: ACK the recevied data
		outb(TWCR, (inb(TWCR)&TWCR_CMD_MASK)|BV(TWINT)|BV(TWEA));
	}
	else
	{
		// ackFlag = FALSE: NACK the recevied data
		outb(TWCR, (inb(TWCR)&TWCR_CMD_MASK)|BV(TWINT));
	}

	i2c_WaitForComplete();

	// retieve received data byte from i2c TWDR
	return( inb(TWDR) );
}
