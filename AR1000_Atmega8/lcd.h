
#define		LCD_DDR		DDRB
#define 	LCD_PORT	PORTB

#define LCD_DC                 0x01  //  PB0
#define LCD_CE                 0x04  //  PB2
#define SPI_MOSI               0x08  //  PB3
#define LCD_RST                0x10  //  PB4
#define SPI_CLK                0x20  //  PB5



void LCD_init(void);
void LCD_clear(void);
void LCD_write_english_string(unsigned char X,unsigned char Y,char *s);
void LCD_write_inverse_string(unsigned char X,unsigned char Y,char *s);
void LCD_write_chinese_string(unsigned char X, unsigned char Y,
                   unsigned char ch_with,unsigned char num,
                   unsigned char line,unsigned char row);                 

void LCD_write_char(unsigned char c);
void LCD_write_byte(unsigned char data, unsigned char dc);
