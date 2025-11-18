/*
    9-27-09
    Copyright Spark Fun Electronics© 2009
    Nathan Seidle
    spark at sparkfun.com

    Example Interface to AR1000
	Using ATmega168 at 8MHz
*/ 

#include <stdio.h>
#include <avr/io.h>
#include "i2c.h"

#define FOSC 8000000
#define BAUD 9600
#define MYUBRR 103

#define sbi(port, bit_mask)   ((port) |= (uint8_t)(1 << bit_mask))
#define cbi(port, bit_mask)   ((port) &= (uint8_t)~(1 << bit_mask))

#define STATUS_LED 5

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

//I2CDEBUG will turn on '!' serial characters and TWI status debugging
//Comment out the define to turn off debug characters
//#define I2CDEBUG 

//Define functions
//======================
void i2c_SendStart(void);
void i2c_SendStop(void);
void i2c_WaitForComplete(void);
unsigned char i2c_SendByte(unsigned char data);
unsigned char i2c_ReceiveByte(unsigned char ackFlag);

void ioinit(void);
static int uart_putchar(char c, FILE *stream);
uint8_t uart_getchar(void);
int uart_gethex(uint8_t length_to_read);

static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);

void delay_ms(uint16_t x); 
void delay_us(uint16_t x);

uint16_t ar1000_read(uint8_t address);
void ar1000_write(char reg_address, uint16_t reg_value);
void ar1000_readall(void);
void ar1000_write_array(void);
void ar1000_init(void);
void ar1000_setvolume(uint8_t volume_level);
void ar1000_status(void);
void ar1000_tuneto(uint16_t freq_kHz);
void ar1000_tune_hilo(uint16_t freq_kHz);
void ar1000_seek(void);
uint16_t ar1000_rssi(void);

void print_array(void);
void set_array_value(void);

//RSSI 49-54 is pretty good. 34-41 is complete static

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
	
	//Bad values from the Airoha example code - they don't work for me
	/*0xFF7B,	// R0 -- the first writable register .  (disable xo_en)
	0x5B15,		// R1
	0xD0B9,		// R2 
	0xA010,		// R3  seekTHD = 16
	0x0780,		// R4
	0x28AB,		// R5
	0x6400,		// R6
	0x1EE7,		// R7
	0x7141,		// R8
	0x007D,		// R9
	0x82C6,		// R10  disable wrap
	0x4F55,		// R11. <--- (disable xo_output)
	0x970C,		// R12.
	0xB845,		// R13
	0xFC2D,		// R14 : Volume control 2
	0x8097,		// R15
	0x04A1,		// R16
	0xDF6A	*/  // R17
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

//======================

int main (void)
{
	ioinit(); //Setup IO pins and defaults
	
	char option;
	char vol = 21;
	
	while(1)
	{
		printf("\n\n----AR1000 Configuration----");
		printf("\n1) Send array values");
		printf("\n2) Read array values");
		printf("\n3) Set array value");	
		printf("\n4) Read All Register values");
		printf("\n5) Begin Seek");		
		printf("\n6) Tune to 97.3");		
		printf("\n7) Check Status");		
		printf("\n8) Raise volume");		
		printf("\n9) Lower volume");		
		printf("\na) Check RSSI");		
		printf("\nb) Special tune hi/low to 97.3");		

		option = uart_getchar();
		
		if(option == '1')
		{
			printf("\n\n\tSending Array Values\n");
			ar1000_write_array();
			printf("\n\t\tDone\n");
		}
		
		else if(option == '2')
		{
			print_array();
		}
		
		else if(option == '3')	
		{
			set_array_value();
		}
		
		else if(option == '4')
		{
			printf("\n\n\tRead All\n");
			ar1000_readall();
		}
		
		else if(option == '5')
		{
			ar1000_seek();
		}
		
		else if(option == '6')
		{
			printf("\n\t Tune to 97.3\n");
			ar1000_tuneto(973);
		}

		else if(option == '7')
		{
			ar1000_status();
		}

		else if(option == '8')
		{
			if(vol < 21) ar1000_setvolume(vol++);
			printf("\nVolume = %02d", vol);
		}
		else if(option == '9')
		{
			if(vol > 0) ar1000_setvolume(vol--);
			printf("\nVolume = %02d", vol);
		}
		else if(option == 'a')
		{
			option = ar1000_rssi();
			printf("\nRSSI = %d", option);
		}
		else if(option == 'b')
		{
			ar1000_tune_hilo(973);
		}

		else
			printf("\n\nChoice = %c", option);
	}
	
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

// Volume Control
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
		printf("!");
	}
	
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

	printf("\nLow complete");

	uint16_t status = ar1000_read(ADDR_RSSI);
	uint16_t rssi = (status & MASK_RSSI);

	printf("\nRSSI 1 = %d", rssi >> SHIFT_RSSI);

	//Read Hi-Side LO Injection
	// R11-->set D15, set D0/D2,  D3 is the same as default
	temp = ar1000_read(11) | 0x8005;
	ar1000_write(11, temp);
	
	//TUNE to FreqKHz with current setting
	ar1000_tuneto(freq_kHz); //This function turns on TUNE and waits for STC flag
	//High-side TUNE Ends 

	printf("\nHigh complete");
	

	status = ar1000_read(ADDR_RSSI);
	printf("\nRSSI 2 = %d", (status & MASK_RSSI) >> SHIFT_RSSI);
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

	printf("\nLow/Hi tuning complete");

}

//Starts scanning the stations for a minimum set threshold. I found the bit to enable
//wrapping so the every time the function is called, it searches up, and wrap back to 88MHz
//if it doesn't find a good station.
void ar1000_seek(void)
{
	#define ADDR_SEEK_SETTING	0x11
	#define SEEK_SETTING	0x2000
	#define SEEK_MASK		0xC3FF
	#define SEEK_TH_MASK	0xFF80
	#define SEEK_TH			5 //A higher threshold causes stronger stations to be found
	
	uint16_t temp;
	
	char space = 1; //0.1MHz scanning
	char updown = 1; //Seek up
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

	printf("\n\nBegin searching:");

	AR1000_SEEK_ON;
	temp = ar1000_read(3);
	if(space == 1)	temp |= (1<<13); //Set space
	if(updown == 1)	temp |= (1<<15); //Set seek up or down
	temp = (temp & 0xE7FF) | band; //Set Band
	temp &= SEEK_TH_MASK; //Clear out the seek threshold
	temp |= SEEK_TH; //Set threshold
	ar1000_write(3, temp);
	
	//Wait for tune to stabilize (STC flag)
	temp = 0;
	while(temp == 0)
	{
		temp = ar1000_read(ADDR_STATUS) & MASK_STC;
		printf(".");
	}

	temp = ar1000_read(ADDR_STATUS) & MASK_SF;
	if(temp != 0)
	{
		printf("\nSeek failed!");
		return;
	}
	printf("\nSeek success!");
	
	temp = ar1000_read(ADDR_STATUS) & MASK_READCHAN;
	uint16_t freq_kHz = 690 + (temp >> SHIFT_READCHAN); //Determine what channel we found
	
	printf("\nNow on channel %d.%dMHz", freq_kHz / 10, freq_kHz % 10);

	//Restore setting after seek
	ar1000_write(17, register_values[17]);

	//Fine-tune with auto hilo rejection
	ar1000_tune_hilo(freq_kHz);

	AR1000_MUTE_OFF;     
}

//This function reads in an address and array value, and stores the value into the array address
//it does not change the values in the ar1000, you will need to use the "send array valus"
//function in order to send the new values to the ar1000	
void set_array_value(void)
{
	uint8_t array_address;
	uint16_t array_value;
	
	printf("\n\nArray Address (2 digit hex value): ");
	array_address = uart_gethex(2);
	printf("\nArray Address is: %x", array_address);
    
   	printf("\n\nArray value (4 digit hex value): ");
	array_value = uart_gethex(4);
    printf("\nArray value is: %x", array_value);
    
    register_values[array_address] = array_value;
}

void print_array(void)
{
	//prints the values of the array that is used to control the radio
	
	printf("\n\nArray position: value\n");

	for(int i = 0 ; i < 18 ; i++)
		printf("0x%.2X: 0x%.4X\n", i, register_values[i]);
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
		#ifdef I2CDEBUG
			printf("!"); //No ACK!
		#endif
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
		printf("\n\tAck failed!");
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
	{
		#ifdef I2CDEBUG
			printf("!"); //No ACK!
		#endif
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
		printf("0x%.2X: 0x%.4X\n", x, register_value);
	}
}

void ar1000_init(void)
{
	delay_ms(100); //Wait for power to stabilize
	
	ar1000_write_array(); //Init the AR1000 by writing the initial recommended values

	uint16_t status = 0;
	while(status == 0)
	{
		status = ar1000_read(ADDR_STATUS);
	
		printf("\nAR1000 status : 0x%04X", status);

		status = status & MASK_STC;
		delay_ms(10);
	}
}

//Read the status register (0x13) of the AR1000
void ar1000_status(void)
{
	uint16_t status;
	
	status = ar1000_read(ADDR_STATUS);
	
	printf("\n\nAR1000 status : 0x%04X", status);
	
	uint16_t channel = status & MASK_READCHAN;
	channel >>= SHIFT_READCHAN;
	channel += 690;
	printf("\nChannel : %02d.%01dMHz", channel / 10, channel % 10);
	
	if(status & MASK_STC) 
		printf(" (Seek/Tune Complete)");
	else
		printf(" (Seek/Tune Incomplete)");
	
	if(status & MASK_SF) 
		printf(" (Seek Fail)");
	else
		printf(" (Seek Successful)");
		
	if(status & MASK_ST) 
		printf(" (Stereo)");
	else
		printf(" (Mono)");
}

//Setup for UART and IO pins
void ioinit (void)
{
	
	//1 = output, 0 = input
    DDRB = 0b11111111; //All outputs
    DDRC = 0b11111111; //All outputs
    //DDRD = 0b11111110; //PORTD (RX on PD0)
	stdout = &mystdout; //Required for printf init

	UBRR0H = (MYUBRR) >> 8;
	UBRR0L = MYUBRR;

	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	UCSR0C = (3<<UCSZ00);
	UCSR0A = (1<<U2X0);

	//Init timer 0 for delay_us timing
	//8,000,000 / 8 = 1,000,000
    TCCR0B = (1<<CS01); //Set Prescaler to 8. CS01=1
	
	//initialize I2C hardware
	TWCR = 0x00;
	TWBR = 64;
	//TWSR = (1 << TWPS1);
	cbi(TWCR, TWEA);
	sbi(TWCR, TWEN);
	
	ar1000_init(); //Initialize AR1000
}

static int uart_putchar(char c, FILE *stream)
{
    if (c == '\n') uart_putchar('\r', stream);
  
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
    
    return 0;
}

uint8_t uart_getchar(void)
{
    while( !(UCSR0A & (1<<RXC0)) );
    return(UDR0);
}

//Reads in ASCII characters, converts to decimal and returns decimal variable
//length_to_read is how many characters we want to read, "A33C" would be four characters long
int uart_gethex(uint8_t length_to_read)
{
	int decimal_value = 0;
	unsigned char input_character;

	for(uint8_t x = 0 ; x < length_to_read ; x++)
	{
		decimal_value <<= 4; //Every time we loop, make room for another 4-bit hex value to be masked into decimal_value
		
		input_character = uart_getchar();
		printf("%c", input_character);	

		//Check the character for validity
		if( (input_character >= '0') && (input_character <= '9') ) 
			decimal_value |= (input_character - '0');
		
		else if( (input_character >= 'a') && (input_character <= 'f') )
			decimal_value |= (input_character - 'a');
		
		else
		{
			//We have a non-legal character. Handle the exception however you want
			return(0);
		}
	}

	return(decimal_value);
}

//General short delays
//general short delays
//Uses internal timer do a fairly accurate 1us
//Because we are using 16MHz and a prescalar of 8 on Timer0, we have to double x
void delay_us(uint16_t x)
{
	while(x > 256)
	{
		TIFR0 = (1<<TOV0); //Clear any interrupt flags on Timer0
		TCNT0 = 0; //Preload Timer0 for 256 clicks. Should be 1us per click
		while( (TIFR0 & (1<<TOV0)) == 0);
		
		x -= 256;
	}

	TIFR0 = (1<<TOV0); //Clear any interrupt flags on Timer0
	TCNT0 = 256 - x; //256 - 125 = 131 : Preload Timer0 for x clicks. Should be 1us per click
	while( (TIFR0 & (1<<TOV0)) == 0);
}

//General short delays
void delay_ms(uint16_t x)
{
	for ( ; x > 0 ; x--)
		delay_us(1000);
}

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
	
	#ifdef I2CDEBUG
		printf("twst: 0x%02X\n\n", twst);
	#endif
	
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
