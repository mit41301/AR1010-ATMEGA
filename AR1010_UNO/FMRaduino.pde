// "FM-Raduino" with Sparkfun's AR1000 radio module
//  by Musashinodenpa 2009

#include "Wire.h"
#include "MsTimer2.h"

#define AR1000ADRS 0x10  // I2C slave 7bit(100_0000)
#define	RSSI	0x12     // AR1000 register
#define	STATUS	0x13
#define BASEFREQ 690  // 69.0MHz is the lowest freq.

#define SRDATA 16     // 74AC164 data pin
#define SRCLK 17      // clock pin
#define SINKPIN 2     // 4 pins (the lowest pin number is defined)
#define KEYPADPINX 6  // 3 pins
#define KEYPADPINY 9  // 4 pins

#define VOLUMECONTROL 10 // keycode
#define PRESETCALL 11

#define FREQCHANGE 1
#define VOLCHANGE 2
#define FREQINPUT1 3
#define FREQINPUT2 4

#define PRESETMAX 5
int preset[PRESETMAX] = {
  761, 795, 800, 813, 825      // 800=80.0MHz
};

byte vol1[22] = {
  0xF,0xF,0xF,0xF,0xF,0xE,0xE,0xD,0xD,0xB,0xA,
  0x9,0x7,0x6,0x6,0x5,0x5,0x3,0x3,0x2,0x1,0x0
};
byte vol2[22] = {
  0x0,0xC,0xD,0xE,0xF,0xE,0xF,0xE,0xF,0xF,0xF,
  0xF,0xF,0xE,0xF,0xE,0xF,0xE,0xF,0xF,0xF,0xF
};

byte LEDpattern[4];
int freq = 825;


void setup(void) {
  pinMode(SRDATA, OUTPUT);
  pinMode(SRCLK, OUTPUT);

  for(int i = SINKPIN; i < SINKPIN + 4; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  Wire.begin();
  Serial.begin(9600);

  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  // writing default values (from the datasheet)
  writeReg(0, 0xffff);
  writeReg(1, 0x5b15);
  writeReg(2, 0xf400);
  writeReg(3, 0x8012);
  writeReg(4, 0x0400);
  writeReg(5, 0x28AA);
  writeReg(6, 0x4400);
  writeReg(7, 0x1EE7);
  writeReg(8, 0x7141);
  writeReg(9, 0x007D);
  writeReg(10, 0x82CE);
  writeReg(11, 0x4F55);
  writeReg(12, 0x970C);
  writeReg(13, 0xB845);
  writeReg(14, 0xFC2D);
  writeReg(15, 0x8097);
  writeReg(16, 0x04A1);
  writeReg(17, 0xDF6A);

  Serial.println("AR1000 enabled.");

  MsTimer2::set(4, refreshLED);
  MsTimer2::start();

  initKeypad();
  setVolume(5);
  setPattern(FREQCHANGE, freq);
}


void loop() {
  static unsigned int lastv, v;

  v = analogRead(0);          // tuning by dial(pot)
  v = v / 4;
  if(lastv != v && v < 250) { // changed? not high end?
    lastv = v;
    freq = BASEFREQ + v;
    tune(freq);
    setPattern(FREQCHANGE, freq);
  }
  delay(50);

  char k = scanKeypad();
  if(k != -1) keypadIF(k);    // keydown detected
}


void tune(unsigned int v) {
  unsigned int r;

  writeReg(2, 0xF400);     // clear to change freq
  writeReg(2, 0xF600 | (v - BASEFREQ)); // set REG2_TUNE to new freq
  do {
    delay(10);
    r = readReg(STATUS);
  } while(!(r & 0x0020));    // waiting for completion

  Serial.print(v, DEC);
  Serial.print(" = ");
  r = readReg(RSSI);         // reading signal strength
  r = (r & 0x7e00)>>8;
  Serial.println(r, DEC);    // sending a RSSI value
}

void keypadIF(char k) {
  static int volume = 5;   // 0-9
  static char p = 0;       // current preset freq

  delay(25);
  if(scanKeypad() != k) return;
  while(scanKeypad() != -1) {}     // wait for key-up

  if(k == VOLUMECONTROL) {
    setPattern(VOLCHANGE, volume); // indicate current level as prompt
    do {
      k = scanKeypad();            // wait for second key
    } while(k == -1);
    delay(25);
    while(scanKeypad() != -1) {}

    if(k > 9) k = 0;               // ignoring * and #
    volume = k;
    setVolume(volume);
    setPattern(FREQCHANGE, freq);

  } else if(k == PRESETCALL) {
    if(++p == PRESETMAX) p = 0;
    tune(preset[p]);
    freq = preset[p];
    setPattern(FREQCHANGE, preset[p]);

  } else if(k > 5) {               // input new freq with keypad
    setPattern(FREQINPUT1, k);
    freq = k * 100;

    do {
      k = scanKeypad();
    } while(k == -1);
    delay(25);
    while(scanKeypad() != -1) {}
    setPattern(FREQINPUT2, k);
    freq += k * 10;

    do {
      k = scanKeypad();
    } while(k == -1);
    delay(25);
    while(scanKeypad() != -1) {}
    freq += k;

    tune(freq);
    setPattern(FREQCHANGE, freq);
  }
}

void initKeypad() {
  for(char x = KEYPADPINX; x < KEYPADPINX + 3; x++) {
    pinMode(x, INPUT);
    digitalWrite(x, HIGH);  // internal pull-up
  }
  for(char y = KEYPADPINY; y < KEYPADPINY + 4; y++) {
    pinMode(y, OUTPUT);
    digitalWrite(y, HIGH);
  }
}

// scanning 4x3 keypad
char scanKeypad() {
  static char keyCode[4][3] = {
    11,0,10, 9,8,7, 6,5,4, 3,2,1  // upside down...
  };
  for(char y = 0; y < 4; y++) {
    digitalWrite(KEYPADPINY + y, LOW);
    for(char x = 0; x < 3; x++) {
      if(digitalRead(KEYPADPINX + x) == LOW) {
        digitalWrite(KEYPADPINY + y, HIGH);
        return(keyCode[y][x]);
      }
    }
    digitalWrite(KEYPADPINY + y, HIGH);
  }
  return(-1);
}

// 4 digits 7segments LED display
void refreshLED() {
  static byte digit;

  digitalWrite(SINKPIN + digit % 4, LOW);
  ++digit;

  shiftOut(SRDATA, SRCLK, LSBFIRST, LEDpattern[digit % 4]);
  digitalWrite(SINKPIN + digit % 4, HIGH);  // select a FET
}

void setPattern(unsigned char mode, int num) {
  static byte font[10] = {
	//abcdefg.
    0b11111100,
    0b01100000,
    0b11011010,
    0b11110010,
    0b01100110,
    0b10110110,
    0b10111110,
    0b11100100,
    0b11111110,
    0b11100110
  };

  switch(mode) {
    case FREQCHANGE:
    LEDpattern[3] = 0b00000000;  // leftmost digit is blank
    LEDpattern[2] = font[(num / 100) % 10];
    LEDpattern[1] = font[(num / 10) % 10] | 0b00100000;
    LEDpattern[0] = font[num % 10];
    break;

    case VOLCHANGE:
    LEDpattern[3] = 0b00011010;  // 'L'
    LEDpattern[2] = 0b00000000;
    LEDpattern[1] = font[num];
    LEDpattern[0] = 0b00000000;
    break;

    case FREQINPUT1:
    LEDpattern[3] = 0b10001110;  // 'F'
    LEDpattern[2] = font[num];
    LEDpattern[1] = 0b00010000;
    LEDpattern[0] = 0b00010000;
    break;

    case FREQINPUT2:             // F82._
    LEDpattern[1] = font[num];
    break;

    default:
    break;
  }
}

void writeReg(byte reg, unsigned int data) {
  Wire.beginTransmission(AR1000ADRS);
  Wire.send(reg);
  Wire.send((uint8_t)highByte(data));
  Wire.send((uint8_t)lowByte(data));
  Wire.endTransmission();
}

unsigned int readReg(byte reg){
  unsigned int data;

  Wire.beginTransmission(AR1000ADRS);
  Wire.send(reg);
  Wire.endTransmission();

  Wire.beginTransmission(AR1000ADRS);
  Wire.requestFrom(AR1000ADRS, 2);
  while(!Wire.available()) {
  }
  data = Wire.receive();
  data <<= 8;
  data |= Wire.receive();

  return data;
}

void setVolume(int v) {
  unsigned int r;

  r = readReg(1);
  writeReg(1, r | 0x0200);   // hardware mute

  r = readReg(3);
  writeReg(3, (r & ~0x0780) | (vol1[v * 2] << 7));

  r = readReg(14);
  writeReg(14, (r & ~0xF000)| (vol2[v * 2] << 12));

  r = readReg(1);
  writeReg(1, r &= ~0x0200); // hardware mute disable
}
