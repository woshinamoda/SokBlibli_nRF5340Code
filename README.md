# nRF5340_Softdevice_note_Sosssk
Record my verification code in Bilibili column tutorial<br>

## update 2024-04-18
新增分支LBS<br>
主要：学习到lesson4的内容，LBS点灯/按键服务知识<br>
> 工程新增文件<br>
- [x] src-> my_lbs.c
- [x] src-> my_lbs.h
    #### 1. main.新建内容<br>
    * 新增连接参数修改（PHY、MTU、连接间隔）
    * 更新发生才连接回调事件内on_connected（），在BT回调事件里面添加更新成功打印log的功能函数
    * notice ： prj要定义FPU和更新连接参数的配置
    * mian.c中新建一个线程（send_data_thread_id），用来定时模拟传感器发送数据
    #### 2. lbs.c 内容<br>
    * 参考DevAcademy教程，添加按键点灯服务
    * 服务下新增特征
      * 1 点灯特征 属性：write
      * 2 按键特征 属性：indicate & read 
      * 3 传感器特征 属性：notify(可选择使能描述符)



##  update 2024-04-14
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