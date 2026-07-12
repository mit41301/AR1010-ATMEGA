// liman324@yandex.ru rcl-radio.ru


#ifndef HT4x14LED_H
#define HT4x14LED_H

#define HT4x14LED_address 0x70

#include <Arduino.h>
class HT4x14LED
{
  public:
    HT4x14LED();
    void init(bool st);
     // 1 >> ON      
     // 0 >> STANDBY 
    void brig(byte br);
     // Brightness 0...15
    void blink(byte bl);
     // 0 >> DISPLAY_OFF       
     // 1 >> DISPLAY_ON        
     // 2 >> BLINK_ON_0.5HZ    
     // 3 >> BLINK_ON_1HZ      
     // 4 >> BLINK_ON_2HZ   
     void clear(); 
     void print(int data_int, byte dp   ,int r0, int r1, int r2, int r3);  
		
 private:
	void writeWire(byte a);
};
	
#endif //HT4x14LED_H
