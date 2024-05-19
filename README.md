# nRF5340_Softdevice_note_Sosssk
Record my verification code in Bilibili column tutorial<br>
## update 2024-05-19
����NFC 00B������Թ���<br>
���������ļ�
- [x] src-> nfct_oob.c
- [x] src-> nfct_oob.h

prj.conf��������<br>
* 1 POLL:����TNEP�¼���ѯ
* 2 SMP��������Կ���SMP��ȫЭ��
* 3 NFC����أ�NDEF, TNEP, T4T, NDEF_LE_OOB, NDEF_TEXT�ȵȣ�
  
nfct_oob����˼·
> /***********pair key perpare���************/<br>
> ��main�ж�ȡ�����ص�ַ�����ݸ���ʱ��ԿTK����ʼ��&pair_signal��ѯ�¼����¼�����׼����<br>
> �������Կ��ͬʱ��ֵ��&oob_local<br>
>  /***********sercurity connģ��************/<br>
> ����ð�ȫ�����---�����֤�ص��¼�����֤�ɹ������Ϣ�ص��¼��ĳ�ʼ����ֵ��������
>  /***********tnep tag oobģ��************/<br>
> �����tnep�л������¼��ṹ�壬������䡮�յ���������ص��¼���<br>
> tnep��ʼ����ͨ��nfc_tnep_initial_msg_encode��ʼ��message����Ϊtnep<br>
> �������г������tnep�������л�����ͬʱ�����һ������text��record��message<br>
> �������ʼ������ģ��<br>

����SMP����֪ʶ���ο��ĵ���nordic Bվ��Ƶ���Լ�nordic������ѧԺBLE��5�γ�

ע���
* 1 ����ȡ����ADD��Ҫ������˶�ȡ�����Կ�һЩhci��ͷ�ļ�����nRF53˫�ˣ�
* 2 �������������Ժ�Ͳ���Ҫpoll��paring_key_process�Ȳ���
![�������](picture/NFCOOB�������.png)
![���ӳɹ�](picture/���������ϴӻ�.png)
## update 2024-05-04
������������ȡ���ܣ�ͬʱ���NUS�������ݽ�������<br>
���������ļ�
- [x] src-> ads1299.c
- [x] src-> ads1299.h

���������߼�����
* 1������1299����������
* 2������gpiote���ܣ���1299�ⲿ����(DRDY)�����ط�����꣬Ȼ����send_data_thread_id�߳�����ͨ��spi��ȡ1299���ݣ����ѹ�����
* 3��ѭ���������Ƿ�Ϊ�գ���Ϊ�մӶ�������ȡ�����ݣ�ͨ��NUS RX�������ͳ�ȥ
* 4��NUS TX������ӻ�ȡ��������յ��ַ���'T' ��ʼ�ɼ����ݣ� �յ��ַ���'R'ֹͣ�ɼ�����

ע��Ҫ��
* notice 1���������ⲿ�����ж϶�ȡ���ݣ���Ȼspi�ᱨtimeout err���²�gpiote�ж����ȼ�5��ϵͳ����thread���ȼ����ڴˣ���������һ����ӣ�֮ǰ���ж϶�����һֱ�Ǵ���ġ�
* notice 2��1299Ӳ�������ֻ��1Ƭ����ʹ�þջ�����START���ſ������ߡ��������ͣ��ڳ�������ʹ��start����

ʵ�ʲ��Թ���
> ��ջѹ��ѹ���߼�������ȷ<br>
> ���ݶ�ȡ��������<br>
> spiʱ����ȷ����ο���ͼ<br>


�߼������ǲ���ͼ
�ɼ�1299 8��ͨ�����ݣ�1��ͨ��3bytes����һ��bytes��STAT_LOFF������Ȥ�Լ��Ķ�1299�ֲ���˵��������һ���ɼ�����4*8+3 = 27bytes
![logic](picture/1299ʱ��ͼ.png)

�������ݣ���NUS��������һ��OOB��NFC��ԣ�BLE������ᣬ����Blibli���ݣ������Լ������õ�������Ŀֱ��copy�������ѧϰ�������nRF7002��nRF5340��ESB����ʵ�ʲ�Ʒ����ͨ����������������Э��


## update 2024-04-29
����FEM����<br>
��ӷ�ʽ�Ƚϼ�<br>
��child_image�������͵�conf��overlay<br>
> *ʹ��FEM��Ҫ�õ�nordic��MPSL�⣬����Kconfig��������<br>

hci_ipc.conf���������
```C+
CONFIG_MPSL=y
CONFIG_MPSL_FEM=y
CONFIG_MPSL_FEM_SIMPLE_GPIO=y
CONFIG_BT_CTLR_TX_PWR_ANTENNA=14
```
> *ͬʱ����˵��豸�������FEM���ŵ�ַ�ڵ㶨��

hci_ipc.ocerlay���������
```C
/ {
    nrf_radio_fem: name_of_fem_node {
       compatible  = "nordic,nrf21540-fem";
       tx-en-gpios = <&gpio0 30 GPIO_ACTIVE_HIGH>;
       rx-en-gpios = <&gpio0 31 GPIO_ACTIVE_HIGH>;
       pdn-gpios   = <&gpio1 10 GPIO_ACTIVE_HIGH>;
 };
};
```
> *ע��㣺��nRF5340���棬�����Ҫ������������ţ���ҪӦ�ú����ͷ����ţ�����Ӧ�ú�Ҳ��Ҫ����overlay

Ӧ�ú˵�overlay����������£������ͷ����ŵ������
```C
&gpio_fwd{
    /delete-node/ uart;
    nrf21540-gpio-if{
        gpios = <&gpio0 30 0>,
        <&gpio0 31 0>,
        <&gpio1 10 0>,
        <&gpio1 11 0>;
    };
};
```
��������2��K_timer��ʱ���Ļص�������������жϣ������������Ժ���ָʾ�ƣ������Ͽ�����˸ָʾ�ơ�

 *��д�ܽ᣺* ʵ�ʲ������FEMǰ���ֻ���ȡRSSI = -80dBm�����FEM���ȡRSSI = -25dBm
 
[![device](picture/FEM.png)]



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
| picture                                          | ��Ҫչʾ�Ĳ���ͼƬ
| nrf5340dk_nrf5340_cpuapp.overlay                 | �޸��Զ����豸��
| child_image                                      | ��image��conf�޸��ļ�

[![device](picture/DEVICE.png)]
