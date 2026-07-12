#ifndef AR1010_LIB
#define AR1010_LIB

#include <stdint.h>


// Write/read methods -------------------------------------------
void writeToRegister(uint8_t address, uint16_t mydata);
uint16_t readFromRegister(uint8_t address);

// Other methods ------------------------------------------------
void AR1010_initialise();

void AR1010_setSeekThreshold(uint8_t threshold);

void AR1010_setFrequency(uint16_t frequency);
void AR1010_setVolume(uint8_t volume);
void AR1010_setHardmute(uint8_t hmute);
void AR1010_setSoftmute(uint8_t smute);

uint16_t frequency();
uint16_t AR1010_seek(char direction);        // 'u' - up, 'd' - down

void setBitInRegister(uint8_t address, uint8_t bitnum, uint8_t bitState);
void setSeekDirection(char direction);        // 'u' - up, 'd' - down

uint8_t readLOInjection(uint8_t loHi);                // Returns stronger signal - 0 = low-side, 1 = high-side
void tuneWithAutoHiLo();
#endif