#include 	<avr/io.h>
#include 	<avr/interrupt.h>
#include 	<string.h>
#include 	<compat/twi.h>

#include 	"mydefs.h"
#include 	<util/delay.h>
//---------------------------------------------------------------------------------//
uint8_t TWI_start (void)
{
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTA); 

    while (!(TWCR & (1<<TWINT)));

    switch (TW_STATUS) 
	{        
        case TW_START:          
        case TW_REP_START:      
            return 1;
        
        case TW_MT_ARB_LOST:    
        default:
            return 0;		    
    }
	_delay_us (5);
}
//-------------------------------------------------------------------------------//
void TWI_stop (void)
{
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO); 
	_delay_us (5);
}
//-------------------------------------------------------------------------------//
uint8_t TWI_read(uint8_t ack_bit)
{
    
	if (ack_bit){
        TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
    } else {
        TWCR = (1<<TWINT)|(1<<TWEN);    
    }
	
	while (!(TWCR & (1<<TWINT)));
	
    switch (TW_STATUS) 
	{  
        case TW_MR_DATA_ACK:    
        
        case TW_MR_DATA_NACK:   
            break;        
       
        case TW_MR_ARB_LOST:    
        default:
            return 0;
    }
	_delay_us (5);
    return(TWDR);       
}
//-------------------------------------------------------------------------------//
uint8_t TWI_send(uint8_t uc_data,uint8_t ack_bit)
{    
    TWDR = uc_data;     

    if (ack_bit){
        TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
    } else {
        TWCR = (1<<TWINT)|(1<<TWEN);    
    }

    while (!(TWCR & (1<<TWINT)));

    switch (TW_STATUS) {
        case TW_MT_SLA_ACK:     
 
        case TW_MT_SLA_NACK:    
            return 1;

        case TW_MR_SLA_ACK:     

        case TW_MR_SLA_NACK:    
            return 2;

        case TW_MT_DATA_ACK:    

        case TW_MT_DATA_NACK:   
            return 3;    

        case TW_MT_ARB_LOST:    
        default:
            return 0;
    }
	_delay_us (5);
}
