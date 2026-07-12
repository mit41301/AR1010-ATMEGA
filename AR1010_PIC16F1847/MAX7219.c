#include "MAX7219.h"

void SPI_Write_Byte(unsigned short num){
 unsigned short t, Mask, Flag;
 CLK_Pin = 0;
 Mask = 0x80;
 for (t=0; t<8; t++){
  Flag = num & Mask;
  if(Flag == 0) MOSI_Pin = 0;
  else MOSI_Pin = 1;
  CLK_Pin = 1;
  CLK_Pin = 0;
  Mask = Mask >> 1;
 }
}


void SPI_Write_Reg( char regadd, char val ){

  CS_Pin = 0;                    // CS pin is pulled LOW
  SPI_Write_Byte(regadd);             // Send thousands digit
  SPI_Write_Byte(val);
  CS_Pin = 1;                    // CS pin is pulled HIGH
  
  }


void MAX7219_INIT(char scanlimit, char brightness) {
  
  char i;
  
  SPI_Write_Reg( shutdown , enable );
  SPI_Write_Reg( decode_mode , 0xff );
  SPI_Write_Reg( intensity , brightness );
  SPI_Write_Reg( scan_limit , scanlimit );
  SPI_Write_Reg( display_test, enable ); delay_ms(200);
  SPI_Write_Reg( display_test, disable );
  
  for ( i=1; i<9; i++){
      SPI_Write_Reg(i, 0x0f );
      }

}


 void Display_Frequency(unsigned int j){
 char i;
 char temp;
 
 SPI_Write_Reg( decode_mode , 0xff );

 for ( i=1; i<9; i++){
  SPI_Write_Reg(i, 0x0f );
  }
  
  CS_Pin = 0;                    // CS pin is pulled LOW
  SPI_Write_Byte(6);             // Send thousands digit
  temp =  (j/1000)%10;
  if( temp ) SPI_Write_Byte(temp);
  else SPI_Write_Byte(0x0f);
  CS_Pin = 1;                    // CS pin is pulled HIGH

  CS_Pin = 0;                    // CS pin is pulled LOW
  SPI_Write_Byte(5);             // Send hundreds digit
  SPI_Write_Byte((j/100)%10);
  CS_Pin = 1;                    // CS pin is pulled HIGH

  CS_Pin = 0;                    // CS pin is pulled LOW
  SPI_Write_Byte(4);             // Send tens digit
  SPI_Write_Byte(( (j/10)%10) | 0x80);
  CS_Pin = 1;                    // CS pin is pulled HIGH

  CS_Pin = 0;                    // CS pin is pulled LOW
  SPI_Write_Byte(3);             // Send ones digit
  SPI_Write_Byte(j%10);
  CS_Pin = 1;                    // CS pin is pulled HIGH
 }