#ifndef _ADS1299_H
#define _ADS1299_H


#include "stdint.h"
#include "main.h"


#define SPI1_CS         NRF_GPIO_PIN_MAP(1,12)
#define CS_H            nrf_gpio_pin_set(SPI1_CS)
#define CS_L            nrf_gpio_pin_clear(SPI1_CS)

extern uint8_t	demoIDX;
extern uint8_t eCon_Message_buf[222]; 
extern uint8_t ReadBuf[27];
extern uint8_t writeBuf[27];


//ADS1299 SPI Command Definition Byte Assignments
#define _WAKEUP 		0x02 	// Wake-up from standby mode
#define _STANDBY		0x04 	// Enter Standby mode
#define _RESET 			0x06 	// Reset the device registers to default
#define _START 			0x08 	// Start and restart (synchronize) conversions
#define _STOP 			0x0A 	// Stop conversion
#define _RDATAC 		0x10 	// Enable Read Data Continuous mode (default mode at power-up)
#define _SDATAC 		0x11 	// Stop Read Data Continuous mode
#define _RDATA 			0x12 	// Read data by command supports multiple read back

//ASD1299 Register Addresses
#define ADS_ID			0x3E	// product ID for ADS1299
#define ID_REG  		0x00	// this register contains ADS_ID
#define CONFIG1 		0x01
#define CONFIG2 		0x02
#define CONFIG3 		0x03
#define LOFF			0x04
#define CH1SET 			0x05
#define CH2SET 			0x06
#define CH3SET 			0x07
#define CH4SET 			0x08
#define CH5SET 			0x09
#define CH6SET 			0x0A
#define CH7SET 			0x0B
#define CH8SET 			0x0C
#define BIAS_SENSP      0x0D
#define BIAS_SENSN  	0x0E
#define LOFF_SENSP   	0x0F
#define LOFF_SENSN   	0x10
#define LOFF_FLIP 	    0x11
#define LOFF_STATP  	0x12
#define LOFF_STATN  	0x13
#define GPIO 			0x14
#define MISC1 			0x15
#define MISC2 			0x16
#define CONFIG4 		0x17


/* define end */

typedef enum
{
	SAMPLE_RATE_16000,
	SAMPLE_RATE_8000,
	SAMPLE_RATE_4000,
	SAMPLE_RATE_2000,
	SAMPLE_RATE_1000,
	SAMPLE_RATE_500,
	SAMPLE_RATE_250
}SAMPLE_RATE;

typedef enum
{
	Impedance     =1,
	Normal        =2,
	InternalShort =3,
	TestSignal    =4
}SAMPLE_MODE;

typedef enum
{
	CS_HOLD_ON     =0,
	CS_DEFAULT     =1,
}CS_SET_STATE;

void spi_cs_lock_on(uint8_t lock_state);
void ADS1299_spi_init();
void ADS1299_SampleRate_init(SAMPLE_RATE sr);
void ADS_ModeSelect(SAMPLE_MODE md);
uint8_t ADS_xfer(uint8_t byte);
uint8_t	ADS_WREG(uint8_t address,uint8_t cmd);
uint8_t ADS_RREG(uint8_t address);
void ADS1299_RESET(void);
void ADS_RDATAC(void);
void ADS_SDATAC(void);
uint8_t	ADS_getDeviceID(void);
void ADS1299_START(void);
void ADS_Enter_StandBY();
void ADS_Exit_StandBY();
uint8_t eCon_Checksum(uint8_t *content,uint8_t len);
void updataADS1299_32data(volatile uint8_t *rx_buf_array, size_t rx_buf_length,volatile uint8_t *tx_buf_array, size_t tx_buf_length);
void updateBoardData(void);
void ADS1299_DATA_SetLine();
#endif