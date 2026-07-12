#ifndef MAX7219_H
#define MAX7219_H

// MAX7219 registers


#define    digit_1              0x01
#define    digit_2              0x02
#define    digit_3              0x03
#define    digit_4              0x04
#define    digit_5              0x05
#define    digit_6              0x06
#define    digit_7              0x07
#define    digit_8              0x08

#define    decode_mode          0x09
#define    intensity            0x0a
#define    scan_limit           0x0b
#define    shutdown             0x0c
#define    display_test         0x0f

#define    enable               0x01
#define    disable              0x00

//intensity

#define    max                  0x0f
#define    med                  0x08
#define    mini                 0x00

//scan limit

#define     one                 0b00000001
#define     two                 0b00000011
#define     three               0b00000111
#define     four                0b00001111
#define     five                0b00011111
#define     six                 0b00111111
#define     seven               0b01111111
#define     eight               0b11111111



//MAX7219pin connections

sbit MOSI_Pin     at         RA7_bit;
sbit CS_Pin       at         RA0_bit;
sbit CLK_Pin      at         RA1_bit;

sbit DIN_direction      at      TRISA7_bit;
sbit LOAD_direction     at      TRISA0_bit;
sbit CLK_direction      at      TRISA1_bit;


void SPI_Write_Byte(unsigned short num);

void SPI_Write_Reg( char regadd, char val );

void MAX7219_INIT(char scanlimit, char brightness);

void Display_Frequency(unsigned int j);


#endif