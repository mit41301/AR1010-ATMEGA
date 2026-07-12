//                                          March 7, 2009

#include <16F877.h>
#use delay(clock=10000000)
//#use rs232(baud=9600, xmit=PIN_C6,rcv=PIN_C7)
//#fuses HS, PUT, NOLVP, NOWDT, BROWNOUT, NOMCLR
#fuses HS, PUT, NOLVP, NOWDT, BROWNOUT

#define STATUS_LED PIN_A4
//#DEFINE I2C_CLOCK PIN_B2
//#DEFINE I2C_DATA PIN_B1
#DEFINE I2C_CLOCK PIN_B3
#DEFINE I2C_DATA PIN_B2
//#DEFINE RC_PIN PIN_C4
#DEFINE RC_PIN PIN_A0

#define AR1010_address 16

// The buttons on the remote control (for a Sony TV)
#define RC_1             0
#define RC_2             1
#define RC_3             2
#define RC_4             3
#define RC_5             4
#define RC_6             5
#define RC_7             6
#define RC_8             7
#define RC_9             8
#define RC_0             9
#define RC_POWER        21
#define RC_CHANNEL_UP   16
#define RC_CHANNEL_DOWN 17
#define RC_VOLUME_DOWN  19
#define RC_VOLUME_UP    18
#define RC_MUTE         20
#define RC_ENTER        11
#define RC_PREV_CH      59

#include "lcd1.c"

//Global variables

int16 frequency;
int8 volume = 15;
int1 mute = 0;
// from sparkfun sample code
const int16 register_values[18] = {
   0xffff, // R0  oscilator(bit 7), enable(bit 0)
   0x5b15, // R1  RDS enable(bit 6), RDS interupt, seek tune complete,
           //     de-enp., mono, smute?, hmute? , mute(bit 1)
   0xF4B9, // R2  tune enable(bit 9), channel(bits 0-8) (b9=87.5MHz)
   0x8012, // R3  seek dir.(bit 15),seek(bit 14), space(bit 13), band(bit 12 &11),
           //     volume1(bits 7-10), seek threshold(7 bits)
   0x0400, // R4  ?
   0x28aa, // R5  ?
   0x4400, // R6  ?
   0x1ee7, // R7  ?
   0x7141, // R8  ?
   0x007d, // R9  ?
   0x82ce, // R10 seek wrap
   0x4f55, // R11 AFC
   0x970c, // R12 ?
   0xb845, // R13 stereo LED, interupt LED, LED
   0xfc2d, // R14 volume2(bits 12-15)
   0x8097, // R15 RDS stuff
   0x04a1, // R16
   0xdf6a  // R17
};

/*

// R1
   0x5b15, // R1  RDS enable(bit 6), RDS interupt, seek tune complete,
           //     de-enp., mono, smute?, hmute? , mute(bit 1)
// 1111110000000000
// 5432109876543210
// 0                ?
//  1               seek on/off
//   0110           ?
//       1          tune on/off
//        10        ?
//          0       RDS enable
//           01     ?
//             0    mono
//              1   ?
//               0  mute on/off (1=on)
//                1 ?
// 0101101100010101 0x5b15


R19 status register, readchan (bits 7-15), stereo(bit 3)m seek fail (bit 4)

from manufactures sample code
	0xFFFB,		// R0 -- the first writable register .
	0x5B15,		// R1.
	0xD0B9,		// R2.
	0xA010,		// R3, seekTHD = 16
	0x0780,		// R4
	0x28AB,		// R5
	0x6400,		// R6
	0x1EE7,		// R7
	0x7141,		// R8
	0x007D,		// R9
	0x82C6,		// R10  disable wrap
	0x4E55,		// R11. <---
	0x970C,		// R12.
	0xB845,		// R13
	0xFC2D,		// R14
	0x8097,		// R15
	0x04A1,		// R16
	0xDF6A		// R17
*/

/*
PIN DEFINITION
Terminal No. Name        Description
------------ ----------  -------------------------------------------
  1          ANT         RF input
  2          N.C.
  3          Left Out    Left chanel audio output
  4          Right Out   Right chanel audio output
  5          GND
  6          Vcc
  7          Write/Read  write/read control input for 3Wire Bus (N.C. if I2C)
  8          Busmode     Bus mode select input (GND if I2C )
  9          Clock       Bus clock line input
  10         Data        Bus data line input / output
*/

// Function list
void fm_write_to_register(int8, int16);
int16 fm_read_register(int8);
void fm_init(void);
void fm_seek(void);
void fm_set_frequency(int16);
void i2c_start(void);
void i2c_stop(void);
void i2c_write(int8);
int8 i2c_read(void);
int8 rc(void);

//-------------------------------------------

void main() {

   int8 i,n,command;
   int16 val16, v = 0;

   //blink status LED
   for(i=0;i<2;i++){
      output_low(STATUS_LED);  //on
      delay_ms(100);
      output_high(STATUS_LED);  // off
      delay_ms(100);
   }
   delay_ms(3000); // needed if just powered up

   // initialize devices
   port_b_pullups(TRUE);  // for the i2c lines
   lcd_init();
   lcd_putc('\f');  // clear LCD
   printf(lcd_putc,"LCD OK, ");
   fm_init();
   printf(lcd_putc,"AR1010 OK");

   delay_ms(3000); // needed if just powered up

   // Ottawa radio stations
   //   frequency = 885; // live
   //   frequency = 891; // CHUO
   //   frequency = 899; // Hot
   //   frequency = 907; // CBOF
   //   frequency = 915; // CBC1
   //   frequency = 923; // Jack
   //   frequency = 931; // CKCU
      frequency = 939; // Bob
   //   frequency = 949; // CIMF
   //   frequency = 985; // Jewel
   //   frequency = 991; // CHRI
   //   frequency = 1003; // Majic
   //   frequency = 1011; // Y101
   //   frequency = 1033; // CBC2
   //   frequency = 1053; // Kiss
   //   frequency = 1061; // CHEZ
   //   frequency = 1069; // Virgin
   //   frequency = 1079; // CKDJ

   fm_set_frequency(frequency);
//---------------------------------

//~ test code

#ifdef(FALSE)

   val16 = fm_read_register(19);
   printf(lcd_putc,"%lu", val16);

#endif

//---------------------------------

   while(1){
      command = rc();

      if (command == RC_1){
         fm_set_frequency(915);  // CBC1
         printf(lcd_putc,"\fCBC radio 1");
      }

      else if (command == RC_2){
         fm_set_frequency(931);  // CKCU
         printf(lcd_putc,"\fCKCU");
      }

      else if (command == RC_3){
         fm_set_frequency(939);  // Bob
         printf(lcd_putc,"\fBob FM");
      }

      else if (command == RC_4){
         fm_set_frequency(1003); // Majic
         printf(lcd_putc,"\fMajic 100");
      }

      else if (command == RC_5){
         fm_set_frequency(1053); // Kiss
         printf(lcd_putc,"\fKiss FM");
      }

      else if (command == RC_6){
         fm_set_frequency(1061); // Chez
         printf(lcd_putc,"\fCHEZ 106");
      }

      else if (command == RC_7){
         fm_set_frequency(1011); // Y101
         printf(lcd_putc,"\fY101 (country)");
      }

      else if (command == RC_8){
         fm_set_frequency(1033); // CBC2
         printf(lcd_putc,"\fCBC radio 2");
      }

      else if (command == RC_9){
         fm_set_frequency(1069); // Virgin
         printf(lcd_putc,"\fVirgin radio");
      }

      else if (command == RC_0){
         fm_set_frequency(923); // Jack
         printf(lcd_putc,"\fJack");
      }

      else if (command == RC_MUTE){
         if(mute==0){
            mute = 1;
            // set mute bit (bit 1 of register 1)
            fm_write_to_register(1,0b0101101100010111); // mute
//                                0101101100010101); // default
            lcd_gotoxy(18,2);
            printf(lcd_putc,"\mute");
         }
         else{
            mute = 0;
            // cancel mute
            fm_write_to_register(1,0b0101101100010101); // unmute (normal)
            lcd_gotoxy(18,2);
            printf(lcd_putc,"     ");
         }
      }

      else if (command == RC_VOLUME_DOWN){
          if(volume>0){
             volume -= 1;
             if(volume==10)volume=9;
             if(volume==11)volume=9;
             if(volume==12)volume=9;
             val16 = register_values[3] & 0xF87F; // clear volume bits
             val16 = val16 | ((15-(int16)volume)<<7);
             fm_write_to_register(3, val16); //
             lcd_gotoxy(1,2);
             printf(lcd_putc,"Volume: %2u", volume);
         }
      }

      else if (command == RC_VOLUME_UP){
          if(volume<15){
             volume += 1;
             if(volume==10)volume=13;
             if(volume==11)volume=13;
             if(volume==12)volume=13;
             val16 = register_values[3] & 0xF87F;
             val16 = val16 | ((15-(int16)volume)<<7);
             fm_write_to_register(3, val16);
             lcd_gotoxy(1,2);
             printf(lcd_putc,"Volume: %2u", volume);
         }
      }

      else if (command == RC_POWER){
         printf(lcd_putc,"\fPower button");
      }

      else printf(lcd_putc,"\f%u", command);

      //blink status LED
      for(i=0;i<2;i++){
         output_low(STATUS_LED);  //on
         delay_ms(100);
         output_high(STATUS_LED);  // off
         delay_ms(100);
      }

   } // main loop

} // main


//--------------------------------------

void fm_init(void){

  int i;

  // send default register values to fm chip
  // will still work if only R13, R14 & R0 is sent
  for(i=1; i<=17; i++) fm_write_to_register(i, register_values[i]); // R1 to R17
  // register 0 must be last
  fm_write_to_register(0, register_values[0]);                   // R0

// this line wasn't needed, but now it powers up in mute without this
  fm_write_to_register(1, register_values[1]);                   // R1

}

//---------------------------------------

void fm_set_frequency(int16 freq){

   // frequency is specified in MHz without the decimal point
   // example: 93.1MHz is 931
   // subtract 690 to get the value for CHAN

   //set CHAN
   //set TUNE bit

   // without this line, you can set a frequency only once
   fm_write_to_register(2, (freq - 690) | 0x0000);

   fm_write_to_register(2, (freq - 690) | 0x0200);

}

//---------------------------------------

void fm_write_to_register(int8 register_number, int16 data){

   int8 data_high, data_low;

   data_high = (int8)(data >> 8);      //
   data_low = (int8)(data & 0x00ff);   //

   // 1. Master initiates a START condition.
   i2c_start();

   // 2. Master writes the device address of the slave (AR1000), and then
   //    followed a WRITE bit. Slave sends back an ACK.
   i2c_write(AR1010_address << 1); //

   // 3. Master writes the register address of AR1000. Slave sends back an ACK.
   i2c_write(register_number);

   // 4. Master writes 2-byte data to complete a register, and then sends a STOP
   //    condition to end the write procedure.
   i2c_write(data_high);           // write high byte
   i2c_write(data_low);            // write low byte
   i2c_stop();

}

//---------------------------------------------

// the fuction isn't working yet

int16 fm_read_register(int register_number){
int8 data_high, data_low;
int16 data;

// numbered comments are from the manual

// 1. Master (Hostprocessor) initiates a START condition.
i2c_start();

// 2. Master writes the device address of the slave (AR1000), and then
//    followed by a WRITE bit. Slave sends back an ACK.
i2c_write(AR1010_address << 1 + 0); //

// 3. Master writes the register address of AR1000. Slave sends back an ACK.
i2c_write(register_number);

// 4. Master re-initiates a start condition.
i2c_start();

// 5. Master writes the device address of the slave (AR1000) again, and
//    then followed a READ bit. Slave sends back an ACK.
//i2c_write((AR1010_address << 1) + 1); //
i2c_write(0x21); //

// 6. Master sends CLOCK signal into slave, and slave outputs associated bit
//    data at DATA pin. Master sends ACK at the end of each byte data.
delay_ms(50);
data_high = i2c_read();           // read high byte
delay_ms(50);
data_low = i2c_read();            // read low byte
delay_ms(50);

// 7. After 2 bytes data read from slave, master sends a STOP condition to
//    end the read procedure.
i2c_stop();

// combine bytes
data = (int16)data_high;
data = (data<<8) + (int16)data_low;

return data;

}

//---------------------------------------------

void fm_seek(void){

/*
proceedure from the AR1010 manual
- Set hmute Bit
- Clear TUNE Bit
- Clear SEEK Bit
- Set BAND/SPACE/CHAN Bits
- Enable TUNE Bit
- Wait STC flag (Seek/Tune Complete, in “Status” register)
- Clear hmute Bit
- Update Functions (optional)
*/

//~
//frequency = (fm_read_register(19) >> 7) + 690;

// status register (#19)
// 1111110000000000
// 5432109876543210
// 000000000         READCHAN (current channel number)
//          ?
//           *       Seek/Tune/PowerOn complete
//            *      Seek Fail
//             *     Stereo
//              ???

fm_write_to_register(2,frequency - 690);
// 1111110000000000
// 5432109876543210
// ??????
//       0           TUNE (when set, station changes as specified in CHAN)
//        000000000  CHAN

fm_write_to_register(3,0xa001);    //turn off seek, seek up, set threshold to 1
//fm_write_to_register(3,0xe001);    //turns on seek

fm_write_to_register(3,0b1110000000000001); //turns on seek(bit 15)
//                       1111110000000000
//                       5432109876543210
// 1111110000000000
// 5432109876543210
// 1                seek direction (1=up)
//  1               seek (1=enabled)
//   1              space (1=100kHz, 0=200kHz)
//    00            band (0x=US/Europe band. 87.5 to 108MHz)
//      0000        volume
//          0000001 seek threshold



}

//---------------------------------------------

int8 i2c_read(void){

   int8 n;
//   int1 response;
   int8 data = 0;

   input(I2C_DATA);     // let the line float

   // read 8 bits
   for(n=0;n<8;n++){

      output_high(I2C_CLOCK);
      delay_us(10);

      data = data << 1;
      if(input(I2C_DATA) == 1) {

      data++;
//~
lcd_putc('+');
      }
      delay_us(10);

      output_low(I2C_CLOCK);
      delay_us(10);
   }
   // send an ACK
   output_low(I2C_DATA);    // 0 = ACK
   delay_us(10);

   output_high(I2C_CLOCK);
   delay_us(10);

   output_low(I2C_CLOCK);
   delay_us(10);

   return data;

}

//---------------------------------------------

void i2c_start(){

   output_low(I2C_DATA);
   delay_us(10);
   output_low(I2C_CLOCK);
   delay_us(10);

}

//---------------------------------------------

void i2c_stop(){

   output_high(I2C_CLOCK);
   delay_us(10);
   output_high(I2C_DATA);
   delay_us(10);

}
//---------------------------------------------

void i2c_write(int8 data){

   signed int8 n;
   int1 response;

   // send 8 bits
   for(n=7;n>=0;n--){
      if(bit_test(data,n)) output_high(I2C_DATA);
      else output_low(I2C_DATA);
      delay_us(10);
      output_high(I2C_CLOCK);
      delay_us(10);
      output_low(I2C_CLOCK);
      delay_us(10);
   }
   // wait for ACK
   input(I2C_DATA);     // let the line float
   output_high(I2C_CLOCK);
   delay_us(10);
   response = input(I2C_DATA);    // 0 = ACK
   output_low(I2C_CLOCK);
   delay_us(10);

   if(response == 1) printf(lcd_putc,"no ack!");

// blink if error
#ifdef(0)
   if(response == 1) {

      //blink status LED
      for(n=0;n<2;n++){
         output_high(STATUS_LED);
         delay_ms(1000);
         output_low(STATUS_LED);
         delay_ms(1000);
      }
   }
#endif

}

//---------------------------------------------

int8 rc(void){

   int8 n;
   int8 value = 0;
   int16 t;

   // header:2200us, high:550us, 0:550us, 1:1100us

   //  ____      _   _    _   _   _
   //      |____| |_| |__| |_| |_| |_...
   //              0   1    0   0   0

   /*
   Using a GE universal remote control set to operate a Sony TV.
   To configure: search -> TV -> 002 -> enter

   power         21
   channel up    16
   channel down  17
   volumn down   19
   volumn up     18
   mute          20
   prev chan     59
   #1             0
   #2             1
   #3             2
   #4             3
   #5             4
   #6             5
   #7             6
   #8             7
   #9             8
   #0             9
   enter         11
   */


//   setup_timer_1(T1_INTERNAL|T1_DIV_BY_8); // for 40MHz
   setup_timer_1(T1_INTERNAL|T1_DIV_BY_2); // for 10MHz
   // 0.8us per tick
   // 1us = 1.25 ticks

   while(1){

      start:

      // wait for header
      while(input(RC_PIN)== 0);         // wait for high
      while(input(RC_PIN)==1);          // wait for low
      set_timer1 (0);                   // start timing the header width
      while(input(RC_PIN)==0);          // wait for high
      t = get_timer1() /5 *4;
      if(t<2000 || t>3000) goto start;  // looking for 2500us. start over if not a header
      // read 8 bits
      value=0;
      for(n=0;n<8;n++){

         set_timer1(0);
         while(input(RC_PIN)==1);       // wait for low
         t = get_timer1() /10 *8;       // calculate pulse width in us
         if(t<150 || t>800) goto start; //  looking for 400us (can be <200)
//         if(t<200 || t>800) goto start;  //  looking for 400us
//         if(t<250 || t>600) goto start;  //  looking for 400us

         set_timer1 (0);
         while(input(RC_PIN)==0);    // wait for high
         t = get_timer1() /10 *8;
         if (t>=1800) goto start;       // error
//         if (t<300) goto start;       // error
         if (t<250) goto start;       // error
         if (t>1000) bit_set(value, n);  // looking for 1300us
         else bit_clear(value, n);  // looking for 400us (it's more like 750)
      }
      value = value & 0b01111111; // clear bit 7. data is only 7 bits

      return value;
   } //main loop


} //end
