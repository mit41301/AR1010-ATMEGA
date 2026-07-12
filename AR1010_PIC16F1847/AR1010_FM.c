#include <stdint.h>
#include "AR1010.h"
#include "MAX7219.h"

#define NULL 0


uint16_t frequencyFM = 883;
uint16_t temp = 0;
// temp varibles
bit oldstate, oldstate1, btn1, btn2, btn3, clearbtns, saveEepromFlag;

char txt[15];

void getRegister(uint8_t i){
     temp = readFromRegister(i);
     WordtoHex( temp, txt);
     uart1_write_text( txt );
     uart1_write_text("\r\n");
}
void getAllRegisters(){
     uint8_t i;
     uint16_t temp;
     char txt[6];
     
     for ( i = 1; i<18; i++)
     {
         getRegister(i);
     }
     uart1_write_text("\r\n");
     getRegister(0);

}
void CheckButtons()
{
   if (Button(&PORTA, 2, 10, 1))
    {               // Detect logical one
      oldstate = 1;                              // Update flag
    }
   if (oldstate && Button(&PORTA, 2, 10, 0))
    {   // Detect one-to-zero transition
      Btn1 = 1;
      oldstate = 0;                              // Update flag
     }
   if (Button(&PORTA, 3, 10, 1))
    {               // Detect logical one
      oldstate1 = 1;                              // Update flag
    }
   if (oldstate1 && Button(&PORTA, 3, 10, 0))
    {   // Detect one-to-zero transition
      Btn3 = 1;
      oldstate1 = 0;                              // Update flag
     }
   if (Button(&PORTA, 4, 10, 1))
    {               // Detect logical one
      oldstate = 1;                              // Update flag
    }
   if (oldstate && Button(&PORTA, 4, 10, 0))
    {   // Detect one-to-zero transition
      Btn2 = 1;
      oldstate = 0;                              // Update flag
     }
}
void DisplayNames(){
     char i;
     
     SPI_Write_Reg( decode_mode , 0 );
     
     for ( i=1; i<9; i++){
      SPI_Write_Reg(i, 0x00 );
      }
      SPI_Write_Reg( 7 , 0b01110111);
      SPI_Write_Reg( 6 , 0b00000101);
      SPI_Write_Reg( 5 , 0b00110000);
      SPI_Write_Reg( 4 , 0b01111110);
      SPI_Write_Reg( 3 , 0b00110000);
      SPI_Write_Reg( 2 , 0b01111110);

     Delay_ms(3000);
     SPI_Write_Reg( decode_mode , 0xff );

     for ( i=1; i<9; i++){
      SPI_Write_Reg(i, 0x0f );
      }
}
void AR1010_BCDmode(){
      char i;
     SPI_Write_Reg( decode_mode , 0xff );

     for ( i=1; i<9; i++){
      SPI_Write_Reg(i, 0x0f );
      }
}
void SaveEEprom(uint16_t freq){
     EEPROM_Write( 0 , (uint8_t)(( freq &0xFF00 ) >> 8));
     EEPROM_Write( 1 , (uint8_t)( freq &0x00FF ));
}
void main() {

ANSELA = 0X00;
ANSELB = 0x00;
TRISA = 0;
TRISB = 0;
Trisa2_bit = 1;
Trisa3_bit = 1;
Trisa4_bit = 1;

OSCCON = 0b11110000 ;  // bit7 = 4xpll  | bit3 to 6 = 1110 8mhz intrc | bits 0 to 1 = 00
Delay_ms(5);
Uart1_init(9600);
I2C1_Init(100000);         // initialize I2C communication
Delay_ms(1000);

MAX7219_INIT( 0x07, med);



Uart1_Write_Text("\r\nAR1010 FM\r\n");
FrequencyFM = ( (EEPROM_Read(0) << 8) + EEPROM_Read(1) );

AR1010_initialise();
AR1010_setVolume(18);

DisplayNames(); // display " A R 1 0 1 0 "

AR1010_setFrequency(frequencyFM);
Display_Frequency( frequency() );
wordtostr(frequency(),txt);
uart1_write_text("FREQUENCY = ");  uart1_write_text( txt );      uart1_write_text("\r\n");

Btn1 = 0; Btn2 = 0; Btn3 = 0; clearbtns = 1;

while(1){
 CheckButtons();
 if( saveEepromFlag == 1){SaveEEPROM(frequencyFM); saveEepromFlag = 0; }


 if ( Btn2 ){
    AR1010_seek('u');
    frequencyFM = frequency();
    Display_Frequency( frequencyFM );
    Btn2 = 0;
    saveEepromFlag = 1;
    }
  if ( Btn3 ){
    AR1010_seek('d');
    frequencyFM = frequency();
    Display_Frequency( frequencyFM );
    Btn3 = 0;
    saveEepromFlag = 1;
    }

 }

}