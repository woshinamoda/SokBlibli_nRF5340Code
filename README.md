# nRF5340_Softdevice_note_Sosssk
Record my verification code in Bilibili column tutorial<br>

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
| �ļ�������                | ����     
| -------                  | :--------:  
| src                      | ������
| .gitgnore                | git�ϴ�ȥ��build���֣��û����Լ����SDK���������  
| CMakeLists               | ������   
| prj.conf                 | Kconfig�����ļ�   
| EDA_data                 | ��������ԭ��ͼ