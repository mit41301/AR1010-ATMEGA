//AR 1010
//RSSI 49-54 is pretty good. 34-41 is complete static
//
//

#include "stm32f10x.h"
#include <stdio.h>
#include "ar1001.h"

#define I2C_AR             I2C1
#define I2C_AR_CLK         RCC_APB1Periph_I2C1
#define I2C_AR_GPIO        GPIOB
#define I2C_AR_GPIO_CLK    RCC_APB2Periph_GPIOB
#define I2C_AR_SCL         GPIO_Pin_6
#define I2C_AR_SDA         GPIO_Pin_7
#define I2C_Speed          150000
#define I2C_SLAVE_ADDRESS7 0x20
#define I2C_AR_ADDRESS7    I2C_SLAVE_ADDRESS7

#define ADDR_STATUS	    0x13 // the address of status register
#define MASK_READCHAN   0xFF80 // D7~D15 in address 13H
#define SHIFT_READCHAN  7

#define ADDR_RSSI	    0x12 
#define MASK_RSSI 	    0xFE00
#define SHIFT_RSSI      9

#define SEEK_TH			16   //A higher threshold causes stronger stations to be found
#define SPACE           1	//0.1MHz scanning
#define UPDOWN          1   //Seek up
#define BAND            0   //US/Europe radio band
    
#define AR_SETBITS(__reg,__bits) I2C_AR_WriteRegister(__reg, I2C_AR_ReadRegister(__reg) | (__bits))
#define AR_CLRBITS(__reg,__bits) I2C_AR_WriteRegister(__reg, I2C_AR_ReadRegister(__reg) & ~(__bits))

#define AR1000_MUTE_ON  AR_SETBITS(1, 1<<1)
#define AR1000_MUTE_OFF AR_CLRBITS(1, 1<<1)

#define AR1000_TUNE_ON 	AR_SETBITS(2, 1<<9)
#define AR1000_TUNE_OFF AR_CLRBITS(2, 1<<9)

#define AR1000_SEEK_ON 	AR_SETBITS(3, 1<<14)
#define AR1000_SEEK_OFF AR_CLRBITS(3, 1<<14)

////////////////////////////////////////////////////////////////////////////////////////////////

uint16_t I2C_AR_ReadRegister(uint8_t ReadAddr);
void I2C_AR_WriteRegister(uint8_t WriteAddr, uint16_t Data);
void GPIO_Configuration(void);
void I2C_Configuration(void);

////////////////////////////////////////////////////////////////////////////////////////////////

const uint16_t register_values[18] = 
{	

	0xFFFF, //R0
	0x5B15, //R1
	0xF4B9, //R2 Tune/Channel
//	0x8012, //R3 seekTHD = 18
    (SPACE<<13) | (UPDOWN<<15) | (BAND<<11) | (SEEK_TH), //R3

	0x0400, //R4
	0x28AA, //R5
	0x4400, //R6
	0x1EE7,	//R7
	0x7141,	//R8
	0x007D,	//R9
	0x82C6 | (1<<3),	//R10  enable wrap
	0x4F55,	//R11. <--- (disable xo_output)
	0x970C,	//R12.
	0xB845,	//R13
	0xFC2D,	//R14 : Volume control 2
	0x8097,	//R15
	0x04A1,	//R16
	0xDF6A	//R17

	//Bad values from the Airoha example code - they don't work for me
	//0xFF7B,	// R0 -- the first writable register .  (disable xo_en)
/*    
    0xFFFF,
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
	0xDF6A	  // R17
  */  
};

// volume control (increasing)
const uint8_t AR1000vol[22] =
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

////////////////////////////////////////////////////////////////////////////////////////////////
void wait_stc(void)
{
    #define MAXWAIT 20

    int cnt = MAXWAIT;

    do { 	
        delay_ms ( 100 ); 
        cnt--;
    } while( cnt && (( I2C_AR_ReadRegister(ADDR_STATUS) & MASK_STC ) == 0) ); 

}

////////////////////////////////////////////////////////////////////////////////////////////////
//Reads and prints all 16 registers (16-bits wide) of the AR1000
void ar_readall(void)
{
    printf("\r\nAR: All Registers:\r\n");
    uint16_t x, register_value;
	for(x = 0 ; x < 0x1D ; x++)
	{
		register_value = I2C_AR_ReadRegister(x);
		printf("0x%.2X: 0x%.4X\n\r", x, register_value);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
uint16_t ar_rssi(void)
{
	return( (I2C_AR_ReadRegister(ADDR_RSSI) & MASK_RSSI) >> SHIFT_RSSI);
}
////////////////////////////////////////////////////////////////////////////////////////////////
//Read the status register (0x13) of the AR1000
void ar_printstatus(void)
{
	uint16_t status;
	
	status = I2C_AR_ReadRegister(ADDR_STATUS);
	
	//printf("\r\nAR status : 0x%04X", status);
	
	uint16_t channel = status & MASK_READCHAN;
	channel >>= SHIFT_READCHAN;
	channel += 690;
	printf("\r\nChannel : %02d.%01dMHz", channel / 10, channel % 10);
	
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
    
    printf(" RSSI %d\r\n", ar_rssi());
}
////////////////////////////////////////////////////////////////////////////////////////////////
uint16_t ar_status(void)
{
    return I2C_AR_ReadRegister(ADDR_STATUS);
}
////////////////////////////////////////////////////////////////////////////////////////////////
uint16_t ar_channel(void)
{
    return ar_settings.freq;
}
////////////////////////////////////////////////////////////////////////////////////////////////
//AR schlafen legen
void ar_standby(void)
{
    I2C_AR_WriteRegister(0, register_values[0] & 0xFFFE);
}
////////////////////////////////////////////////////////////////////////////////////////////////
//Aus Standby aufwecken.
//
void ar_wakeup(void)
{
    I2C_AR_WriteRegister(0, register_values[0]);
    wait_stc();
    
    ar_tune_hilo(ar_settings.freq);    

}
////////////////////////////////////////////////////////////////////////////////////////////////
void ar_forcemono(int mono)
{
    ar_settings.mono = mono;

    if (mono)
        AR_SETBITS(1, 1<<3);        
    else
        AR_CLRBITS(1, 1<<3);       
}
////////////////////////////////////////////////////////////////////////////////////////////////
// Volume Control
// There are two different fields about volume control in AR1000F
//  Volume : D7~D10 in register R3
//  Volume2 : D12~D15 in register R14
//  22 combinations of (volume2 + volume) are recommended.
void ar_setvolume(uint8_t volume_level)
{
	uint16_t reg3, reg14;

    if (volume_level > 21) volume_level = 21;
	
    ar_settings.volume = volume_level;
    
	reg3 = I2C_AR_ReadRegister(3) & 0xF87F; //Zero out bits D7-D10
	reg3 |= ( (AR1000vol[volume_level] & 0x0F) << 7); //Mask in D7-D10
	
	reg14 = I2C_AR_ReadRegister(14) & 0x0FFF; //Zero out bits D12-D15
	reg14 |= ((AR1000vol[volume_level] & 0xF0) << 8); //Mask in D12-D15
	
	I2C_AR_WriteRegister(3, reg3);
	I2C_AR_WriteRegister(14, reg14);
    AR1000_MUTE_OFF;
}


////////////////////////////////////////////////////////////////////////////////////////////////
void ar_tune_hilo(uint16_t freq_kHz)
{
    printf("\r\nAR: Finetune");

    ar_settings.freq = freq_kHz;

	AR1000_MUTE_ON; //Set mute ON before TUNE
	AR1000_SEEK_OFF; //Clear seek

    AR1000_TUNE_OFF;
    
	I2C_AR_WriteRegister(2, (I2C_AR_ReadRegister(2) & 0xFE00) | (freq_kHz - 690) ); //Set Frequency    

	//Low-Side LO Injection
	//R11 --> clear  D15,  clear D0/D2,  D3 is the same as default

	I2C_AR_WriteRegister(11, I2C_AR_ReadRegister(11) & 0x7FFA);

	//Enable tune bit
	AR1000_TUNE_ON;
	
	//Wait for tune to stabilize (STC flag)
    wait_stc();	    

	//Low-side TUNE Ends 

    int rssi1 = ar_rssi();
    
    AR1000_TUNE_OFF;
    
	printf(" RSSI 1 = %d", rssi1);

	//Read Hi-Side LO Injection
	// R11-->set D15, set D0/D2,  D3 is the same as default
	I2C_AR_WriteRegister(11, I2C_AR_ReadRegister(11) | 0x8005);
	
	//TUNE to FreqKHz with current setting
	//Enable tune bit
	AR1000_TUNE_ON;
	
	//Wait for tune to stabilize (STC flag)
    wait_stc();	 

    //High-side TUNE Ends 
    int rssi2 = ar_rssi();

    AR1000_TUNE_OFF;
    
	printf(" RSSI 2 = %d", rssi2);
    	
	if (rssi1 > rssi2) //errata in 0.82
	{ 	
		// LO
		// R11--> clear D15, set D0/D2, D3 is the same as default		
		I2C_AR_WriteRegister(11, (I2C_AR_ReadRegister(11) & 0x7FFF) | 0x0005);
	}
	else
	{ 
		//HI
		//R11-->  set D15, clear D0/D2, D3 is the same as default
		I2C_AR_WriteRegister(11, (I2C_AR_ReadRegister(11) | 0x8000) & 0xFFFA);        
	}

    //Enable tune bit
    AR1000_TUNE_ON;
    //Wait for tune to stabilize (STC flag)
    wait_stc();

	AR1000_MUTE_OFF;

	printf(" Low/Hi tuning complete");

}

////////////////////////////////////////////////////////////////////////////////////////////////
//Starts scanning the stations for a minimum set threshold. I found the bit to enable
//wrapping so the every time the function is called, it searches up, and wrap back to 88MHz
//if it doesn't find a good station.
void ar_seek(void)
{
		
    printf("\r\nAR Seek");

	AR1000_MUTE_ON;
	AR1000_TUNE_OFF;
	//AR1000_SEEK_OFF;
	
	printf(" Begin searching: Seek ");	
    	
    AR1000_SEEK_ON;
    
	//Wait for tune to stabilize (STC flag)
    wait_stc();

	int temp = I2C_AR_ReadRegister(ADDR_STATUS) & MASK_SF;
	if(temp != 0)
	{
		printf("failed!");
		return;
	}
	printf("success!");
	
	temp = I2C_AR_ReadRegister(ADDR_STATUS) & MASK_READCHAN;
	int freq_kHz = 690 + (temp >> SHIFT_READCHAN); //Determine what channel we found
	
	//printf(" Now on channel %d.%dMHz", freq_kHz / 10, freq_kHz % 10);

	//Fine-tune with auto hilo rejection
	ar_tune_hilo(freq_kHz);
  
}

////////////////////////////////////////////////////////////////////////////////////////////////
void init_ar(void)
{
    /* I2C Periph clock enable */
    RCC_APB1PeriphClockCmd(I2C_AR_CLK, ENABLE);   
  
    /* GPIO Periph clock enable */
    RCC_APB2PeriphClockCmd(I2C_AR_GPIO_CLK, ENABLE);    
  
    /* GPIO configuration */
    GPIO_Configuration();

    /* I2C configuration */  
    I2C_Configuration();  

    ar_settings.freq = 875;
    ar_settings.mono = FALSE;
    ar_settings.volume = 21;

    int i;
	for(i=1 ; i < 18 ; i++) 
		I2C_AR_WriteRegister(i, register_values[i]); //Write registers 1 to 17 to AR1000
   	
	I2C_AR_WriteRegister(0, register_values[0]); //Re-write the first register, this will set the enable bit
    wait_stc();

    printf("Chip-ID:%.4X Rev:%.1c ",I2C_AR_ReadRegister(0x1c),'A'+(I2C_AR_ReadRegister(0x1b)>>12));
    //ar_readall();
}

/////////////////////////////////////////////////////////////////////////////////
void GPIO_Configuration(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure; 
  
    /* Configure I2C_AR pins: SCL and SDA */
    GPIO_InitStructure.GPIO_Pin =  I2C_AR_SCL | I2C_AR_SDA; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(I2C_AR_GPIO, &GPIO_InitStructure);
}

////////////////////////////////////////////////////////////////////////////////////////////////
void I2C_Configuration(void)
{
    I2C_DeInit(I2C1);

    I2C_InitTypeDef  I2C_InitStructure; 
  
    /* I2C configuration */
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = I2C_SLAVE_ADDRESS7;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = I2C_Speed;
  
    /* I2C Peripheral Enable */
    I2C_Cmd(I2C_AR, ENABLE);
    /* Apply I2C configuration after enabling it */
    I2C_Init(I2C_AR, &I2C_InitStructure);
}

////////////////////////////////////////////////////////////////////////////////////////////////
void I2C_AR_WriteRegister(uint8_t WriteAddr, uint16_t Data)
{
    //register_values[WriteAddr] =  Data;


    /* Send START condition */
    I2C_GenerateSTART(I2C_AR, ENABLE);

    /* Test on EV5 and clear it */
    while(!I2C_CheckEvent(I2C_AR, I2C_EVENT_MASTER_MODE_SELECT));  

    /* Send EEPROM address for write */
    I2C_Send7bitAddress(I2C_AR, I2C_AR_ADDRESS7, I2C_Direction_Transmitter);
  
    /* Test on EV6 and clear it */
    while(!I2C_CheckEvent(I2C_AR, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
  
    I2C_SendData(I2C_AR, WriteAddr);
  
    /* Test on EV8 and clear it */
    while(!I2C_CheckEvent(I2C_AR, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    /* Send the 1. byte to be written */
  
    uint8_t value1 = (Data & 0xFF00) >> 8;
    uint8_t value2 = (Data & 0x00FF);
  
    I2C_SendData(I2C_AR, value1); 

    /* Test on EV8 and clear it */
    while(!I2C_CheckEvent(I2C_AR, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    /* Send the 2. byte to be written */
    I2C_SendData(I2C_AR, value2); 
   
    /* Test on EV8 and clear it */
    while(!I2C_CheckEvent(I2C_AR, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
    /* Send STOP condition */
    I2C_GenerateSTOP(I2C_AR, ENABLE);
}


////////////////////////////////////////////////////////////////////////////////////////////////
uint16_t I2C_AR_ReadRegister(uint8_t ReadAddr)
{  

    /* While the bus is busy */
    while(I2C_GetFlagStatus(I2C_AR, I2C_FLAG_BUSY));
  
  
    /* Send START condition */
    I2C_GenerateSTART(I2C_AR, ENABLE);
  
  
    /* Test on EV5 and clear it */
    while(!I2C_CheckEvent(I2C_AR, I2C_EVENT_MASTER_MODE_SELECT));
  
  
    /* Send EEPROM address for write */
    I2C_Send7bitAddress(I2C_AR, I2C_AR_ADDRESS7, I2C_Direction_Transmitter);

    /* Test on EV6 and clear it */
    while(!I2C_CheckEvent(I2C_AR, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
  
    /* Send the EEPROM's internal address to read from: Only one byte address */
    I2C_SendData(I2C_AR, ReadAddr);  
  
    /* Test on EV8 and clear it */
    while(!I2C_CheckEvent(I2C_AR, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  
    /* Send STRAT condition a second time */  
    I2C_GenerateSTART(I2C_AR, ENABLE);

    /* Test on EV5 and clear it */
    while(!I2C_CheckEvent(I2C_AR, I2C_EVENT_MASTER_MODE_SELECT));

    /* Send EEPROM address for read */
    I2C_Send7bitAddress(I2C_AR, I2C_AR_ADDRESS7, I2C_Direction_Receiver);
  
    /* Test on EV6 and clear it */
    while(!I2C_CheckEvent(I2C_AR, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));
  
    uint8_t NumByteToRead = 2;
    uint8_t Buffer[2];
  
    /* While there is data to be read */
    while(NumByteToRead)  
    {
  
    if(NumByteToRead == 1)
    {

      /* Disable Acknowledgement */
      I2C_AcknowledgeConfig(I2C_AR, DISABLE);
     
      /* Send STOP Condition */
      I2C_GenerateSTOP(I2C_AR, ENABLE);

    }

    /* Test on EV7 and clear it */
    if(I2C_CheckEvent(I2C_AR, I2C_EVENT_MASTER_BYTE_RECEIVED))  
    {      

      /* Decrement the read bytes counter */
      NumByteToRead--; 
    
      /* Read a byte from the EEPROM */
      Buffer[NumByteToRead] = I2C_ReceiveData(I2C_AR);
       
    }   
    }

    /* Enable Acknowledgement to be ready for another reception */
    I2C_AcknowledgeConfig(I2C_AR, ENABLE);

    return (Buffer[1]<<8 | Buffer[0]);
}
