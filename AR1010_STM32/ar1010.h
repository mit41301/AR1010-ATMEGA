/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef AR1001_H
#define AR1001_H

#include "stm32f10x.h"

#define MASK_STC (1<<5) //0x0020  //  Seek/Tune/PowerOn complete  D5 in  adress 13H 
#define MASK_SF	 (1<<4) //0x0010  //  Seek Fail D4 in address 13H
#define MASK_ST	 (1<<3) //0x0008  //  Stereo  D3 in address 13H

void init_ar(void);

void ar_readall(void); //debug: Register Anzeigen

void ar_printstatus(void);
void ar_setvolume(uint8_t volume_level);
void ar_seek(void);
void ar_tune_hilo(uint16_t freq_kHz);  //finetune

void ar_forcemono(int mono); // True: Mono, False: Stereo
void ar_standby(void);
void ar_wakeup(void);


uint16_t ar_rssi(void);    // Gibt RSSI zurück
uint16_t ar_status(void);  // Gibt Status zurück
uint16_t ar_channel(void); // Gibt Frequenz zurück


struct {
    uint16_t freq;
    uint8_t  volume;
    uint8_t  mono;
} ar_settings;

#endif