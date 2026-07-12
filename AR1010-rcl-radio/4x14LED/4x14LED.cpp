#include <Arduino.h>
#include <Wire.h>
#include "4x14LED.h"

HT4x14LED::HT4x14LED(){Wire.begin();}

void HT4x14LED::init(bool st){writeWire(0x20|st);}
void HT4x14LED::brig(byte br){writeWire(0xE0|br);}
void HT4x14LED::blink(byte bl){
   switch(bl){
     case 0: bl = 0x80;break; // off
     case 1: bl = 0x81;break; // on
     case 2: bl = 0x87;break; // 0.5 Hz
     case 3: bl = 0x85;break; // 1 Hz
     case 4: bl = 0x83;break; // 2 Hz
}
  writeWire(bl);
}

void HT4x14LED::clear(){
  Wire.beginTransmission(HT4x14LED_address);
  Wire.write(0x00);
  for(byte i_x=0;i_x<8;i_x++){Wire.write(0);}
  Wire.endTransmission();  
} 

void HT4x14LED::print(int data, byte dp,   int r0, int r1, int r2, int r3){
  Wire.beginTransmission(HT4x14LED_address);
  Wire.write(0x00); 
  byte data_arh[4];
  data_arh[0] = data/1000;
  data_arh[1] = data/100%10;
  data_arh[2] = data/10%10;
  data_arh[3] = data%10;
  byte h0[4],h1[4];
  for(byte i_x=0;i_x<4;i_x++){
    switch(data_arh[i_x]){
     case 0: h0[i_x] = 0x3F;h1[i_x]=0x0C;break;   // 0
     case 1: h0[i_x] = 0x06;h1[i_x]=0x04;break;   // 1
     case 2: h0[i_x] = 0xDB;h1[i_x]=0x00;break;   // 2
     case 3: h0[i_x] = 0x8F;h1[i_x]=0x00;break;   // 3
     case 4: h0[i_x] = 0xE6;h1[i_x]=0x00;break;   // 4
     case 5: h0[i_x] = 0xED;h1[i_x]=0x00;break;   // 5
     case 6: h0[i_x] = 0xFD;h1[i_x]=0x00;break;   // 6
     case 7: h0[i_x] = 0x01;h1[i_x]=0x14;break;   // 7
     case 8: h0[i_x] = 0xFF;h1[i_x]=0x00;break;   // 8
     case 9: h0[i_x] = 0xEF;h1[i_x]=0x00;break;   // 9
  }}
  
  if(r0>=0){ h1[0] = (r0 & 0xFF00) >> 8; h0[0] = r0 & 0x00FF;}
  if(r1>=0){ h1[1] = (r1 & 0xFF00) >> 8; h0[1] = r1 & 0x00FF;}
  if(r2>=0){ h1[2] = (r2 & 0xFF00) >> 8; h0[2] = r2 & 0x00FF;}
  if(r3>=0){ h1[3] = (r3 & 0xFF00) >> 8; h0[3] = r3 & 0x00FF;}

  switch(dp){
    case 0: break;
    case 1: h1[0] = h1[0]+0b01000000;break;
    case 2: h1[1] = h1[1]+0b01000000;break;
    case 3: h1[2] = h1[2]+0b01000000;break;
    case 4: h1[3] = h1[3]+0b01000000;break;
    }
  for(byte i_x=0;i_x<4;i_x++){Wire.write(h0[i_x]); Wire.write(h1[i_x]);}
  Wire.endTransmission();  
}  


//////// private //////////////////////////////////////////////////////////////////////////////////////
void HT4x14LED::writeWire(byte a){
  Wire.beginTransmission(HT4x14LED_address);
  Wire.write (a);
  Wire.endTransmission();
}

