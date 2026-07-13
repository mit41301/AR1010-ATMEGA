/*
 *  TDA.c
 *  
 *
 *  Created by Peter Loy on 4/26/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

/*
 Notes:
 SDA = C1
 SCL = C0
 
 SET SA_DATAOUT(PIN 13 ON TDA) TO VCC!!!!!!!!!!!!!
 
 SENDING:
 

ORDER OF STUFF TO BE SENT ON DATAIN:
 pllreg0[7:0]
 pllreg1[7:0]
 pllreg2[7:0]
 pllreg3[7:0]
 pllreg4[7:0]
 testreg[7:0]
 sinc4reg[7:0]
 rds_bd_ctrl[7:0]
 rds_int[7:0]
 
 Receiving:

 
 ORDER OF STUFF SENT ON DATAOUT:
 rds_int[7:0]
	-bit 4 indicates synchronization (1 = synchronized, 0 = synchronization in progress)
	-bit 6 indicates if buffer is empty (0 = empty) (BNE bit)
 rds_qu[7:0]
 rds_corrp[7:0]: 
	-bit 0 indicates if the the buffer has overflowed (1 = overflow)
	-bit 1 indicates if RDS data should be used (1 = good)
	-bit 2 indicates error correction (1 = error correction used)
 rds_bd_l[7:0] = bits 0:7 of actual RDS info
 rds_bd_h[7:0] = bits 8:15 of actual RDS info
 rds_bd_ctrl[7:0]
	-bit 0: (1 = write into buffer only if synchronized), (0 = write into buffer any incoming RDS block)
	-bit 1: (1 = PLL time constants set by software), (0 = PLL time constants set by hardware)
	-bit 2: set 57 kHz PLL time constant
	-bit 3: bit 0 of 1187.5 Hz PLL time constant
	-bit 4: bit 1 of 1187.5 Hz PLL time constant
	-bit 7: select internal master clock frequency (1 = 8.664 MHz, 0 = 8.55 MHz <-reset value)
 sinc4reg[7:0] = always fill with 0's
 testreg[7:0] = always fill with 0's
 pllreg4[7:0]
 pllreg3[7:0]
 pllreg2[7:0]
 pllreg1[7:0]
 pllreg0[7:0]

 */

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "types.h"
#include "defs.h"
#include "i2c.h"
#include <util/delay.h>

#define sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

#define SLA_W 0x22 //0b00100010 write address
#define SLA_R 0x23 //0b00100011 read address

#define RDS_INT		0
#define RDS_QU		1
#define RDS_CORRP	2
#define RDS_BD_H	3
#define RDS_BD_L	4
#define RDS_BD_CTRL	5
#define	SINC4REG	6
#define TESTREG		7
#define PLLREG4		8
#define	PLLREG3		9
#define	PLLREG2		10
#define	PLLREG1		11
#define	PLLREG0		12

//Define functions
//***********************************************************
void i2c_SendStart(void);
void i2c_SendStop(void);
void i2c_WaitForComplete(void);
unsigned char i2c_SendByte(unsigned char data);
unsigned char i2c_ReceiveByte(unsigned char ackFlag);

void TDAInit(void);//***DONE***

void TDAread(void);// ***DONE***
void TDAwrite(int index, char reg_value);//***DONE***
void TDAinitWrite(void);

// debugging function to figure stuff out
void decode(void);
void blinkLed1(char num);

// Data to be written/read
char data[13]; //Index corresponds to order of registers; order give above

// RDS data 
int RDS_index = 0;
char RDS_data_h[24];
char RDS_data_l[24];

/*
 Data type notes:
 
 A = PI code
 B = 4-bit group type + 1-bit B0 + 1-bit Traffic Program ID + 5-bit Program Type ID + 5-bit ?????
	B0 bit gives version (A or B) => Affects group C
 C = depends 
 D = depends
 */
char RDS_data_type[24];

//Debugging LEDs
char led;

//***********************************************************
/*
void main(void){
	// wait for synch
	_delay_ms(1);
	
	mega_init();
	
	// Check buffer for data and store it
	while(1){
		// Check INTN pin (active low): CONNECT INTN PIN TO A PIN ON THE STK (I USED PIN B.0)!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		if (~PINB0){
			TDAread();
		}
	}
}
*/

//***********************************************************
void TDAInit(void){
	// Initialize data with reset values of TDA registers
	data[0] = 0x00;
	data[1] = 0x00;
	data[2] = 0x00;
	data[3] = 0x00;
	data[4] = 0x00;
	data[5] = 0x01;
	data[6] = 0x00;
	data[7] = 0x00;
	data[8] = 0x0C;
	data[9] = 0x06;
	data[10] = 0xE9;
	data[11] = 0xC2;
	data[12] = 0x80;

	// Force a resynch of TDA
	TDAwrite(RDS_INT, 0x20); //0b00100000
	
	// Initialize data
	data[0] = 0x02;
	data[1] = 0x00;
	data[2] = 0x00;
	data[3] = 0x00;
	data[4] = 0x00;
	data[5] = 0x81;
	data[6] = 0x00;
	data[7] = 0x00;
	data[8] = 0x2C;  //pll4
	data[9] = 0x04;  //pll3
	data[10] = 0x20; //pll2
	data[11] = 0x44; //pll1
	data[12] = 0x66; //pll0
	
	TDAinitWrite();
	// Set itsrc0 bit = 1 => sets interrupt on buffer not empty
	TDAwrite(RDS_INT, 0x02); //0b00000010
}

//***********************************************************
void TDAread(void){
	char ack;
	
AGAIN:
	i2c_SendStart();

	ack = i2c_SendByte(SLA_R);
	if(ack == 0)
	{
		goto AGAIN;
	}

	// check BNE bit of rds_int
	blinkLed1(0x55);
	data[RDS_INT] = i2c_ReceiveByte(ack);
	blinkLed1(0xaa);
	
	if (((data[RDS_INT]>>6) & 0x1) == 0){ // Buffer is empty
		i2c_SendStop();
	}
	else { // Buffer is not empty
		data[RDS_QU] = i2c_ReceiveByte(TRUE);
		data[RDS_CORRP] = i2c_ReceiveByte(TRUE);
		data[RDS_BD_H] = i2c_ReceiveByte(TRUE);
		data[RDS_BD_L] = i2c_ReceiveByte(TRUE);
		i2c_SendStop();
		
		// update RDS_data arrays
		RDS_data_h[RDS_index] = data[RDS_BD_H];
		RDS_data_l[RDS_index] = data[RDS_BD_L];
		RDS_data_type[RDS_index] = (data[RDS_QU]>>2) & 0x03;

		decode();
		
		// increment RDS_index
		RDS_index++;
		
		// check if buffer is now empty, if not, repeat process
		goto AGAIN;
	}
	
	// Reset RDS_index
	RDS_index = 0;
}

//***********************************************************
void TDAwrite(int index, char reg_value){
	// This is setup to only write to the first 2 registers (rds_int(0) and rds_bd_ctrl(5))
	// I don't think we will ever write to the PLL registers and we don't change the sinc4reg or testreg registers
	
	char ack;
	
	// Update data array
	data[index] = reg_value;
	
AGAIN:
	i2c_SendStart();
	ack = i2c_SendByte(SLA_W);
	ack &= i2c_SendByte(data[0]);
	ack &= i2c_SendByte(data[5]);
	i2c_SendStop();
	
	if(ack == 0)
	{
		goto AGAIN;
	}
}

//***********************************************************
void TDAinitWrite(void){
	// Writes the initial values we want (For setting PLL, etc.)
	char ack;
	
AGAIN:
	i2c_SendStart();
	ack = i2c_SendByte(SLA_W);
	ack &= i2c_SendByte(data[0]);
	ack &= i2c_SendByte(data[5]);
	ack &= i2c_SendByte(data[6]);
	ack &= i2c_SendByte(data[7]);
	ack &= i2c_SendByte(data[8]);
	ack &= i2c_SendByte(data[9]);
	ack &= i2c_SendByte(data[10]);
	ack &= i2c_SendByte(data[11]);
	ack &= i2c_SendByte(data[12]);
	i2c_SendStop();
	
	if(ack == 0)
	{
		goto AGAIN;
	}
}

//***********************************************************
void decode(void) {
	// Should display the group type code
	if (RDS_data_type[RDS_index] == 0x01){
		led = RDS_data_h[RDS_index]>>3;
		PORTB = ~led;
	}
}

void blinkLed1(char num){
	_delay_ms(500);
	led = led^num;
	PORTB = led;
	_delay_ms(500);
	led = led^num;
	PORTB = led;
}


//***********************************************************

/*
 I2C Functions
 */

//***********************************************************
void i2c_SendStart(void)
{
	// send start condition
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	
	i2c_WaitForComplete();
}

//***********************************************************
void i2c_SendStop(void)
{
	// transmit stop condition
	TWCR = (1<<TWSTO);
}

//***********************************************************
void i2c_WaitForComplete(void)
{
	// wait for any previous i2c stuff to complete before sending new data
	while (!(TWCR & (1<<TWINT)));
}

//***********************************************************
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

//***********************************************************
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


/* IDEAS:
 1.  Write a general read and (separate) write function
 2.  Write specific read/write functions for each register on TDA (using general read/write)
 3.  Write an init function for the TDA????
 */

