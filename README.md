# nRF5340_Softdevice_note_Sosssk
Record my verification code in Bilibili column tutorial<br>

## **day update by 2024-04-14** 
记录基于BLE协议栈的调试代码（peripheral-uart）<br>
笔记思路
1. 广播（响应，广播参数更新）
2. 链接（CLE, DLE, PHY, 连接参数更新）
3. 数据透传（重发，缓存）



## **Create the first repository** 
专栏前8节内容有关nRF5340外设知识，比较简单<br>
> * 1 - nRF5340硬件学习的资料汇总
> * 2 - 设备树初步学习汇总
> * 3 - NCS组成架构
> * 4 - 在VScode中打印程序调试log
> * 5 - 串口外设
> * 6 - IIC外设
> * 7 - SPI + USB ACM CDC外设
> * 8 - NFCT外设
---
> 文档内容可以参考blibli专栏介绍<br>
[本人专栏链接](https://member.bilibili.com/platform/upload-manager/opus) 
---
### 文件夹目录内容说明<br>
| 文件夹名称                | 内容     
| -------                  | :--------:  
| src                      | 主代码
| .gitgnore                | git上传去掉build部分，用户子自己添加SDK编译会生成  
| CMakeLists               | 编译项   
| prj.conf                 | Kconfig配置文件   
| EDA_data                 | 嘉立创的原理图