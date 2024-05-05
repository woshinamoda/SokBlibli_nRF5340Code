#include "ads1299.h"
#include <zephyr/drivers/spi.h>

#define LOG_MODULE_NAME Sok_1299_log
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

// spi �ڵ����� + �豸�ṹ��ָ��
#define spi0_NODE   DT_NODELABEL(spi0)
static const struct device *spi4_dev = DEVICE_DT_GET(spi0_NODE);

// set spi config �ṹ��
struct spi_config spi_cfg;
struct spi_cs_control cs_ctrl;

// user private code and variable
uint8_t ads1299_id = 0x00;
uint8_t	demoIDX=0;
uint8_t packet_cnt;
uint8_t eCon_Message_buf[222] = {0xBB,0xAA}; 
uint8_t ReadBuf[27];
uint8_t writeBuf[27]={0x00,0x00};
/*�ⲿ��������*/
void ads1299_data_prepare();

void spi_CS_Lock_on(uint8_t lock_state)
{
    switch(lock_state)
    {
        case 0: spi_cfg.operation = SPI_WORD_SET(8)|SPI_MODE_CPHA|SPI_HOLD_ON_CS;
            break;
        case 1: spi_cfg.operation = SPI_WORD_SET(8)|SPI_MODE_CPHA&(~(SPI_HOLD_ON_CS));
            break;
    }
}

// ads1299 spi���ó�ʼ������
void ADS1299_spi_init()
{
	cs_ctrl.gpio.port = DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(spi4), cs_gpios));
	cs_ctrl.gpio.pin = DT_GPIO_PIN(DT_NODELABEL(spi4), cs_gpios);
	cs_ctrl.gpio.dt_flags = DT_GPIO_FLAGS(DT_NODELABEL(spi4), cs_gpios);
	cs_ctrl.delay = 2;
	spi_cfg.operation = SPI_WORD_SET(8)|SPI_MODE_CPHA| SPI_OP_MODE_MASTER;
	spi_cfg.frequency = 8000000;
	spi_cfg.cs = cs_ctrl;
    ADS1299_RESET();
    k_msleep(10);
    ADS_SDATAC();
    k_msleep(10);  
    ads1299_id = ADS_getDeviceID();
    if(ads1299_id != 0x3e)
       	LOG_ERR("ADS1299 init error err: %d", ads1299_id);
    else
		LOG_INF("ADS1299 init success  %d", ads1299_id);
}

//ads1299�����ʳ�ʼ��
void ADS1299_SampleRate_init(SAMPLE_RATE sr)
{
	uint8_t output_data_rate;
	switch(sr) {
			case SAMPLE_RATE_250:
					output_data_rate= 0x06;
					break;
			case SAMPLE_RATE_500:
					output_data_rate= 0x05;
					break;
			case SAMPLE_RATE_1000:
					output_data_rate= 0x04;
					break;
			case SAMPLE_RATE_2000:
					break;
			case SAMPLE_RATE_4000:
					break;
			case SAMPLE_RATE_8000:
					break;
			case SAMPLE_RATE_16000:
					break;
			default:;
	}	
	ADS_WREG(CONFIG1,0xD0 | output_data_rate);		//ע�⣬��Ƭ1299û��ʹ�þջ�������������Ϊ0XD0
	k_msleep(100);
	ADS_WREG(CONFIG3,0xEC);
	k_msleep(100);	
	
}

//����Ads1299ģʽ��1-Impedance��2-Normal��3-Short noise��4-Test wave��
void ADS_ModeSelect(SAMPLE_MODE md)
{
	switch(md) {
		case Impedance:
			{//Impedance-31.2Hz-AC-6nA
				ADS_WREG(LOFF,0x02);
				k_msleep(10);

				ADS_WREG(BIAS_SENSP,0xFF);
				k_msleep(10);
				ADS_WREG(BIAS_SENSN,0xFF);
				k_msleep(10);
				ADS_WREG(LOFF_SENSP,0xFF);
				k_msleep(10);
				ADS_WREG(LOFF_SENSN,0x00);
				k_msleep(10);
				ADS_WREG(LOFF_FLIP,0x00);
				k_msleep(10);
				ADS_WREG(MISC1,0x20);//����SRB1
				k_msleep(10);
				
				ADS_WREG(CH1SET,0x60);
				k_msleep(5);
				ADS_WREG(CH2SET,0x60);
				k_msleep(5);
				ADS_WREG(CH3SET,0x60);
				k_msleep(5);
				ADS_WREG(CH4SET,0x60);
				k_msleep(5);
				ADS_WREG(CH5SET,0x60);
				k_msleep(5);
				ADS_WREG(CH6SET,0x60);
				k_msleep(5);
				ADS_WREG(CH7SET,0x60);
				k_msleep(5);
				ADS_WREG(CH8SET,0x60);
				k_msleep(5);
				
				ADS1299_START();
				k_msleep(5);
				ADS_RDATAC();
				k_msleep(5);
			}
			break;
		case Normal:
			{
				ADS_WREG(CONFIG2,0xC0);
				/*-----------------------------------------------------------------------
				*bit[7:5]			6h
				*bit4:				0:�����ź��ⲿ����							1�������ź��ڲ�����
				*bit3��				0h	
				*bit2��				0��1x-(vrefp-vrefn)/2400				1��2x-(vrefp-vrefn)/2400
				*bit[1:0]			�����ź�Ƶ��
				*----------------------------------------------------------------------*/	
				k_msleep(10);
				ADS_WREG(BIAS_SENSP,0xFF);
				/*-----------------------------------------------------------------------
				  * �üĴ�������ÿ��ͨ�����źŽ���ƫ�õ�ѹBIAS���Ƶ�
					*	BIASPn:						0:�ر����ź�����BIAS��							1��ʹ��
				*----------------------------------------------------------------------*/	
				k_msleep(10);
				ADS_WREG(BIAS_SENSN,0xFF);
				/*-----------------------------------------------------------------------
					* �üĴ�������ÿ��ͨ�����źŽ���ƫ�õ�ѹBIAS���Ƶ�
					*	BIASPn:						0:�ر����ź�����BIAS��							1��ʹ��
				*----------------------------------------------------------------------*/	
				k_msleep(10);
				ADS_WREG(MISC1,0x00);//����SRB1���ش� ����> ���ʹ��
				//ADS_WREG(MISC1,0x20);//����SRB1���رպ� ����> ����ʹ��
				/*-----------------------------------------------------------------------
					*	�üĴ������SRB���Ž��뷴���������
					*	bit5��						0�����ش�													1�����عر�
				*----------------------------------------------------------------------*/	
				k_msleep(10);
				ADS_WREG(CH1SET,0x60);
				/*-----------------------------------------------------------------------
					* CH[1:8]�����ŵ�Դģʽ��PGA����Ͷ�·����������
					*	bit7(power_down)							0����������										1��ͨ���ϵ�		
					*	bit[6:4](PGA gain)						000:1		001:2		010:4		011:6		100:8		101:12		110:24		111:do not use
					*	bit[3]												0: ��SRB2����								1���ر�SRB2����
					*	bit[2:0]��·����MUXn[2:0]			000�������缫����		001�������·		010����BIAS_MEASһ��ʹ�ã����ڲ���ƫ��		011��MVDD����		100���¶��ź�		101�������ź�		110��BIAS_DRP��������������		111��������������
				*----------------------------------------------------------------------*/		
					k_msleep(5);
					ADS_WREG(CH2SET,0x60);	//Gain��24			LSB = 0.02384 uv
					k_msleep(5);
					ADS_WREG(CH3SET,0x60);
					k_msleep(5);
					ADS_WREG(CH4SET,0x60);
					k_msleep(5);
					ADS_WREG(CH5SET,0x60);
					k_msleep(5);
					ADS_WREG(CH6SET,0x60);
					k_msleep(5);
					ADS_WREG(CH7SET,0x60);
					k_msleep(5);
					ADS_WREG(CH8SET,0x60);
					k_msleep(5);	
					ADS1299_START();
					k_msleep(5);
					ADS_RDATAC();
					k_msleep(5);
			}
			break;
		case InternalShort:
			{
				ADS_WREG(CONFIG2,0xC0);
				k_msleep(10);
								
				ADS_WREG(CH1SET,0x01);
				k_msleep(5);
				ADS_WREG(CH2SET,0x01);
				k_msleep(5);
				ADS_WREG(CH3SET,0x01);
				k_msleep(5);
				ADS_WREG(CH4SET,0x01);
				k_msleep(5);
				ADS_WREG(CH5SET,0x01);
				k_msleep(5);
				ADS_WREG(CH6SET,0x01);
				k_msleep(5);
				ADS_WREG(CH7SET,0x01);
				k_msleep(5);
				ADS_WREG(CH8SET,0x01);
				k_msleep(5);

				ADS1299_START();
				k_msleep(5);
				ADS_RDATAC();
				k_msleep(5);
			}
			break;
		case TestSignal:
			{
				ADS_WREG(CONFIG2,0xD0);
				k_msleep(100);
				ADS_WREG(CH1SET,0x05);
				k_msleep(5);
				ADS_WREG(CH2SET,0x05);
				k_msleep(5);
				ADS_WREG(CH3SET,0x05);
				k_msleep(5);
				ADS_WREG(CH4SET,0x05);
				k_msleep(5);
				ADS_WREG(CH5SET,0x05);
				k_msleep(5);
				ADS_WREG(CH6SET,0x05);
				k_msleep(5);
				ADS_WREG(CH7SET,0x05);
				k_msleep(5);
				ADS_WREG(CH8SET,0x05);
				k_msleep(5);
				

				ADS_RDATAC();
				LOG_INF("open ads1299 continuous mode");
				k_msleep(5);
				// ADS1299_START();
				// k_msleep(5);					
			}
			break;
		default:;
	}

}


uint8_t ADS_xfer(uint8_t byte)
{
    int err;
    uint8_t tx_code[1];
    uint8_t rx_code[1];    
    tx_code[0] = byte;
	struct spi_buf spi_tx_buf[1] = {
		{
			.buf = tx_code,
			.len = sizeof(tx_code),
		},
	};  
	struct spi_buf spi_rx_buf[1] = {
		{
			.buf = rx_code,
			.len = sizeof(rx_code),
		},
	};      
	const struct spi_buf_set tx_set = {
		.buffers = spi_tx_buf,
		.count = 1
	};

	const struct spi_buf_set rx_set = {
		.buffers = spi_rx_buf,
		.count = 1
	};     
	err = spi_transceive(spi4_dev, &spi_cfg, &tx_set, &rx_set);
    
    if (err)
    {
		//LOG_ERR("SPI transcation failed err : %d", err);
    }
    else
    {
        return rx_code[0];
       // printk("data : %d", rx_code[0]);        
       // LOG_HEXDUMP_INF(rx_set.buffers->buf, rx_set.buffers->len, "Received SPI data: ");
    }
}

//ADS1299д�Ĵ���
uint8_t	ADS_WREG(uint8_t	address,	uint8_t	cmd)
{
	uint8_t	Opcode1 = address + 0x40;
    spi_CS_Lock_on(CS_HOLD_ON);
	ADS_xfer(Opcode1);
	for(int j=250;j>0;j--){};
	ADS_xfer(0x00);
	for(int j=250;j>0;j--){};
    spi_CS_Lock_on(CS_DEFAULT);
	ADS_xfer(cmd);
	for(int j=250;j>0;j--){};
}

//ADS1299���Ĵ���
uint8_t ADS_RREG(uint8_t address)
{
    uint8_t reback_data;
	uint8_t	Opcode1 = address + 0x20;
    spi_CS_Lock_on(CS_HOLD_ON);  					
	ADS_xfer(Opcode1);
	for(int j=250;j>0;j--){};
	ADS_xfer(0x00);
	for(int j=250;j>0;j--){};
    spi_CS_Lock_on(CS_DEFAULT);	    
	reback_data = ADS_xfer(0x00);
    //printk("The ADS1299 chip ID is: %d", reback_data); 
	for(int j=250;j>0;j--){};		
	return 	reback_data;				
}

//ADS1299�����λ
void ADS1299_RESET(void)
{
    //CS_L;
	ADS_xfer(_RESET);
	k_msleep(1);
    //CS_H;
}

//ADS������ȡģʽ
void ADS_RDATAC(void)
{
    //CS_L;
    ADS_xfer(_RDATAC);
    for(int j=250;j>0;j--){};
    k_msleep(1);
    //CS_H;
}

//ADS1299ֹͣ������ȡģʽ
void ADS_SDATAC(void)
{
    //CS_L;
    ADS_xfer(_SDATAC);
    for(int j=250;j>0;j--){};
    k_msleep(1);
    //CS_H;
}

//ADS��ȡоƬID
uint8_t	ADS_getDeviceID(void)
{
    uint8_t chip_id;
    //CS_L;
    chip_id = ADS_RREG(ID_REG);
    return chip_id;

}


void ADS1299_STOP(){
    ADS_xfer(_STOP);
    for(int j=250;j>0;j--){};
}


//ADS�����ź�
void ADS1299_START(void)
{
    //CS_L;
    ADS_xfer(_START);
    for(int j=250;j>0;j--){};
    //CS_H;
}

//ADS����StandBYģʽ
void ADS_Enter_StandBY()
{
    //CS_L;
    ADS_xfer(_STANDBY);
    for(int j=250;j>0;j--){};
    k_msleep(1);
    //CS_H;
}

//ADS�Ƴ�StandBYģʽ
void ADS_Exit_StandBY()
{
    //CS_L;
    ADS_xfer(_WAKEUP);
    for(int j=250;j>0;j--){};
    k_msleep(1);
    //CS_H;
}

//У�飺���ݺ���
uint8_t eCon_Checksum(uint8_t *content,uint8_t len)
{
    uint8_t result = 0;
    for(int i=0;i<len;i++) {
        result += content[i];
    }
    return ~result;
}

/*-----------------------------��ȡ1Ƭ1299����---------------------------------------
1Ƭ1299��ȡ������ = 8chn x 3bytes = 24bytes
1������ 9֡ ������ = 24 x 9 = 216
���ϸ������ݸ�ʽ = 216 + 2 + 4 = 222bytes
�������ݸ�ʽ ���� *eCon_Message_buf
	 0-1			2 - 217			218			219			 220			 221
	BBAA	+	8ch*3bytes*9pot	+	У��	+	��ǩ	+	��ص���	+	�����cnt
-----------------------------------------------------------------------------------------*/
void updataADS1299_32data(volatile uint8_t *rx_buf_array, size_t rx_buf_length,volatile uint8_t *tx_buf_array, size_t tx_buf_length)
{
  const struct spi_buf tx_buf[1] = {{.buf = tx_buf_array, .len = tx_buf_length}};
  const struct spi_buf_set tx = {.buffers = tx_buf, .count = 1};
  const struct spi_buf rx_buf[1] = {{.buf = rx_buf_array, .len = rx_buf_length}}; 
  const struct spi_buf_set rx = {.buffers = rx_buf, .count = 1};  
  spi_transceive(spi4_dev, &spi_cfg, &tx, &rx);	  
}


int		boardStat;
void updateBoardData(void)
{
	uint8_t inByte;
	for (int i = 0; i < 3; i++)
	{//3bytes �� STAT
		inByte = ADS_xfer(0x00); 									
		boardStat = (boardStat << 8) | inByte;
	}
	for (int i = 0; i < 8; i++)
	{//8��ͨ��
		for (int j = 0; j < 3; j++)
		{//1������3bytes
			if(i == 0)
			{ 
				inByte = ADS_xfer(0x00);
				eCon_Message_buf[j + i*3 + demoIDX*24] = inByte;
			}
		}
	}
	demoIDX++;
}


void ADS1299_DATA_SetLine()
{
	//updateBoardData();
	updataADS1299_32data(ReadBuf, 27, writeBuf, 27);
	memcpy(&eCon_Message_buf[2+demoIDX*24],&ReadBuf[3], 24);
	demoIDX++;
	if(demoIDX == 9)
	{
		packet_cnt++;
		eCon_Message_buf[0] = 0xBB;
		eCon_Message_buf[1] = 0xAA;		
		eCon_Message_buf[218] = eCon_Checksum(&(eCon_Message_buf[2]),216);	
		eCon_Message_buf[219] = 0;		
		eCon_Message_buf[220] = 100;		
		eCon_Message_buf[221] = packet_cnt;
		demoIDX = 0;
		ads1299_data_prepare();
	}
	
}
