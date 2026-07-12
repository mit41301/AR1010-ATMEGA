#define TW_START					0x08
#define TW_REP_START				0x10
// Master Transmitter
#define TW_MT_SLA_ACK				0x18
#define TW_MT_SLA_NACK				0x20
#define TW_MT_DATA_ACK				0x28
#define TW_MT_DATA_NACK				0x30
#define TW_MT_ARB_LOST				0x38
// Master Receiver
#define TW_MR_ARB_LOST				0x38
#define TW_MR_SLA_ACK				0x40
#define TW_MR_SLA_NACK				0x48
#define TW_MR_DATA_ACK				0x50
#define TW_MR_DATA_NACK				0x58
// Slave Transmitter
#define TW_ST_SLA_ACK				0xA8
#define TW_ST_ARB_LOST_SLA_ACK		0xB0
#define TW_ST_DATA_ACK				0xB8
#define TW_ST_DATA_NACK				0xC0
#define TW_ST_LAST_DATA				0xC8
// Slave Receiver
#define TW_SR_SLA_ACK				0x60
#define TW_SR_ARB_LOST_SLA_ACK		0x68
#define TW_SR_GCALL_ACK				0x70
#define TW_SR_ARB_LOST_GCALL_ACK	0x78
#define TW_SR_DATA_ACK				0x80
#define TW_SR_DATA_NACK				0x88
#define TW_SR_GCALL_DATA_ACK		0x90
#define TW_SR_GCALL_DATA_NACK		0x98
#define TW_SR_STOP					0xA0
// Misc
#define TW_NO_INFO					0xF8
#define TW_BUS_ERROR				0x00

// defines and constants
#define TWCR_CMD_MASK		0x0F
#define TWSR_STATUS_MASK	0xF8

// return values
#define I2C_OK				0x00
#define I2C_ERROR_NODEV		0x01



#define	R0_DEFAULT		0xFFFF
#define	R1_DEFAULT		0x5B15
#define	R2_DEFAULT		0xF4B9
#define	R3_DEFAULT		0x9842

#define	R0_ENABLE		0x0001
#define	R0_XO_EN		0x0002
#define	R1_HMUTE		0x0002
#define	R1_SMUTE		0x0004
#define	R1_MONO			0x0008	// ??
#define	R1_DEEMP		0x0010	// ??	1:75us  0:50us
#define	R1_RDS_EN		0x8000	// ??
#define	R2_CHAN			0x01FF
#define	R2_TUNE			0x0200
#define	R3_SEEKTH		0x007F
#define	R3_VOLUMN		0x0780
#define	R3_BAND			0x1800	// 11:76-108	10:76-90	00:87.5-108
#define	R3_SPACE		0x2000
#define	R3_SEEK			0x4000
#define	R3_SEEKUP		0x8000
#define	R14_VOLUME2		0xF000
#define	STATUS_ST		0x0008
#define	STATUS_SF		0x0010	// ???
#define	STATUS_STC		0x0020
#define	STATUS_READCHAN	0xF800

#define	R0		0x00
#define	R1		0x01
#define	R2		0x02
#define	R3		0x03
#define	R4		0x04
#define	R5		0x05
#define	R6		0x06
#define	R7		0x07
#define	R8		0x08
#define	R9		0x09
#define	R10		0x0A
#define	R11		0x0B
#define	R12		0x0C
#define	R13		0x0D
#define	R14		0x0E
#define	R15		0x0F
#define	R16		0x10
#define	R17		0x11

uint8_t TWI_start (void);
void TWI_stop (void);
uint8_t TWI_read(uint8_t ack_bit);
uint8_t TWI_send(uint8_t uc_data,uint8_t ack_bit);
