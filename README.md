# nRF5340_Softdevice_note_Sosssk
Record my verification code in Bilibili column tutorial<br>

## update 2024-04-24
����NUS��֧��ͬʱmaster�����֧������copy�޸ĺ������ݣ�<br>
> �������Ĺ���<br>

1 . USB CDC�ж�����˫����<br>
```C
uint8_t rx_buffer[2][DATA_SIZE_MAX];
uint8_t *next_buf = rx_buffer[1];
ע��ص��¼�UART_RX_BUF_REQUEST�����л�������
```
2 . ������ʱK_timer<br>
  ��ʱ�Ĺ���ֻ����˸BLEָʾ�ƣ�������������

3 . ��������device_message_queue���Լ������߼�<br>
  * �������ϰ�nRF52һ�����½�һ���ڴ��my_data_array[5000]
  * �Զ���buffer_t�ṹ�壬�ṹ��Ԫ��1.����ͷָ��  2.���ݳ���
  * ͨ��kernel�Դ��Ķ��У�USB���յ����ݸ��Ƶ��ڴ�أ���ָ��ͳ��ȴ���ṹ��ѹ�����
  * Ȼ���ڴӶ���ȡ����Ϣ��ͨ��NUS_Send�����ڷ����Tx�������͸��ֻ�������ˣ�

�������Ż�����<br>
1�����FEM����<br>
2����Ӵ�����������Ҳ�Ǵ����ڴ�أ�������������͸�����<br>



## update 2024-04-21
����master��֧������NUS͸�����񲿷�<br>
1. �����ļ���������child_image��:�����޸���image��config�ļ�
2. ����overlay�ļ������USB CDC��Ҫ���豸�ڵ�
3. main.c�ļ���Ӻ������Զ�̬�޸����Ӳ���le_params<br>
4. ����USB_CDC_ACM���ܣ�����NUS����Ĵ������裬���async_adapter.c�����첽����

���Ӵ���͸������˼·����ԭ��LBS�������޸Ĳ��裩
* 1��conf�����CONFUG_BT_NUS = y��ϵͳ�����nus.c / nus.h�ļ�
* 2�����nus�ķ������ʼ��
* 3��USB_CDC��configע������ʹ��,��ʼ��ȥ���������ȴ�
<br>====================����=======================<br>
�Ѿ�����ʵ��NUS�������ܣ���δ��������շ��߼����������õ�UART0�����ﻻ��USB������child_image�ļ��У���������hci_ipc����˵�config


## update 2024-04-18
������֧LBS<br>
��Ҫ��ѧϰ��lesson4�����ݣ�LBS���/��������֪ʶ<br>
> ���������ļ�<br>
- [x] src-> my_lbs.c
- [x] src-> my_lbs.h
    #### 1. main.�½�����<br>
    * �������Ӳ����޸ģ�PHY��MTU�����Ӽ����
    * ���·��������ӻص��¼���on_connected��������BT�ص��¼�������Ӹ��³ɹ���ӡlog�Ĺ��ܺ���
    * notice �� prjҪ����FPU�͸������Ӳ���������
    * mian.c���½�һ���̣߳�send_data_thread_id����������ʱģ�⴫������������
    #### 2. lbs.c ����<br>
    * �ο�DevAcademy�̳̣���Ӱ�����Ʒ���
    * ��������������
      * 1 ������� ���ԣ�write
      * 2 �������� ���ԣ�indicate & read 
      * 3 ���������� ���ԣ�notify(��ѡ��ʹ��������)



##  update 2024-04-14
��¼����BLEЭ��ջ�ĵ��Դ��루peripheral-uart��<br>
�ʼ�˼·
1. �㲥����Ӧ���㲥�������£�
2. ���ӣ�CLE, DLE, PHY, ���Ӳ������£�
3. ����͸�����ط������棩



## **Create the first repository** 
ר��ǰ8�������й�nRF5340����֪ʶ���Ƚϼ�<br>
> * 1 - nRF5340Ӳ��ѧϰ�����ϻ���
> * 2 - �豸������ѧϰ����
> * 3 - NCS��ɼܹ�
> * 4 - ��VScode�д�ӡ�������log
> * 5 - ��������
> * 6 - IIC����
> * 7 - SPI + USB ACM CDC����
> * 8 - NFCT����
---
> �ĵ����ݿ��Բο�blibliר������<br>
[����ר������](https://member.bilibili.com/platform/upload-manager/opus) 
---
### �ļ���Ŀ¼����˵��<br>
| �ļ�������                                        | ����     
| -------                                          | :--------:  
| src                                              | ������
| .gitgnore                                        | git�ϴ�ȥ��build���֣��û����Լ����SDK���������  
| CMakeLists                                       | ������   
| prj.conf                                         | Kconfig�����ļ�   
| EDA_data                                         | ��������ԭ��ͼ
| nrf5340dk_nrf5340_cpuapp.overlay                 | �޸��Զ����豸��
| child_image                                      | ��image��conf�޸��ļ�