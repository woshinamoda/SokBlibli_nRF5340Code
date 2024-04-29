
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/addr.h>
/*修改数据与连接参数用到*/
#include <zephyr/bluetooth/gatt.h>
/*蓝牙链接头文件*/
#include <zephyr/bluetooth/conn.h>
/*添加nus服务头文件*/
#include <bluetooth/services/nus.h>
/*添加 USB CDC ADC头文件*/
#include <zephyr/usb/usb_device.h>
#include "uart_async_adapter.h"


#include "led_button.h"
#include "main.h"


// 注册加载log模块 
#define LOG_MODULE_NAME Sok_log_init
LOG_MODULE_REGISTER(LOG_MODULE_NAME);
// 自定义广播名称 
#define DEVICE_NAME "Sok_NUS_Service_Test"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
/*  自定义链接设备my_conn
//  Connect management ： bt_conn
//  Zephyr蓝牙协议栈使用bt_conn抽象(*my_conn)来连接其他设备
//  note1：该结构体不向应用程序公开
//  note2：可以使用bt_conn_git_info()获取有限信息
//  note3：可以使用bt_con_ref()确保对象（指针）有效性*/
struct bt_conn *my_conn = NULL;		//用户连接抽象
struct bt_conn *auth_conn = NULL;	//自动重连抽象
// USB CDC ACM UART 缓存
#define DATA_SIZE_MAX  230			//串口（或传感器）一包最多收到230bytes数据
/* 根据节点（zephyr，cdc_acm_uart）获取属性 */
static const struct device *uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);	
typedef struct			//USB收到数据（指令） &&  传感器准备好数据， 缓存到该FIFO中，通过NUS服务发送出去																	
{
	uint8_t * p_data;	//实际存放数据的数组								
	uint16_t length;	//这组数据长度
} buffer_t;
uint8_t rx_buffer[2][DATA_SIZE_MAX];
uint8_t *next_buf = rx_buffer[1];
uint8_t my_data_array[50000];	//缓存USB指令 && 传感器数据区域 eg：50000bytes
uint16_t data_cnt;
K_MSGQ_DEFINE(device_message_queue, sizeof(buffer_t), 200, 2);	//静态创建队列，名称：device_message_queue，大小butter_t，里面消息数量200个，每个元素对齐2bytes


/* throuht send data timer* start***************************************************************/
static uint8_t ble_led_cnt;
static void timer0_handler(struct k_timer *dummy)	//timer tick = 10ms
{
	int err;
	struct bt_conn_info info;
	err = bt_conn_get_info(my_conn, &info);	
	ble_led_cnt++;
	if(ble_led_cnt >= 100)
	{
		if(info.state == BT_CONN_STATE_CONNECTED){
			LED_BLE_L;
			ble_led_cnt = 0;
		}
		else if(info.state == BT_CONN_STATE_DISCONNECTED){
			nrf_gpio_pin_toggle(LED_BLE);
			ble_led_cnt = 0;
		}

	}
}
static K_TIMER_DEFINE(timer0, timer0_handler, NULL);
/* throuht send data timer* end*****************************************************************/


/* 更新连接参数部分代码* start********************************************************************/
static void update_phy(struct bt_conn *conn)	//function 1 : 动态更新radio的phy
{
	int err;
	const struct bt_conn_le_phy_param preferred_phy = {	//发送和接受的调制速率可以单独设置
		.options = BT_CONN_LE_PHY_OPT_NONE,
		.pref_rx_phy = BT_GAP_LE_PHY_2M,
		.pref_tx_phy = BT_GAP_LE_PHY_2M,
	};
	err = bt_conn_le_phy_update(conn, &preferred_phy);
	if (err) {
		LOG_ERR("bt_conn_le_phy_update() returned %d", err);
	}
};
static void update_data_length(struct bt_conn *conn)	//function 2 ： 动态更新数据长度DLE
{
	int err;
	struct bt_conn_le_data_len_param my_data_len = {
		.tx_max_len = BT_GAP_DATA_LEN_MAX,
		.tx_max_time = BT_GAP_DATA_TIME_MAX,
	};
	err = bt_conn_le_data_len_update(my_conn, &my_data_len);
	if (err) {
		LOG_ERR("data_len_update failed (err %d)", err);
	}
};
static struct bt_gatt_exchange_params exchange_params;
static void exchange_func(struct bt_conn *conn, uint8_t att_err,
			  struct bt_gatt_exchange_params *params)
{
	LOG_INF("MTU exchange %s", att_err == 0 ? "successful" : "failed");
	if (!att_err) {
		uint16_t payload_mtu =
			bt_gatt_get_mtu(conn) - 3; // 3 bytes used for Attribute headers.
		LOG_INF("New MTU: %d bytes", payload_mtu);
	}
}
static void update_mtu(struct bt_conn *conn)	//function 3 ： 动态更新MTU
{
	int err;
	exchange_params.func = exchange_func;
	err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (err) {
		LOG_ERR("bt_gatt_exchange_mtu failed (err %d)", err);
	}
}
static void update_led_param(struct bt_conn *conn)	//function 4: 动态更新连接参数
{
	struct bt_le_conn_param param;
	param.interval_max = 800;
	param.interval_min = 800;
	param.latency = 0;
	param.timeout = 400;
	bt_conn_le_param_update(conn, &param);
}
void on_le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,	//更新连接成功回调函数
			 uint16_t timeout)
{
	double connection_interval = interval * 1.25; // in ms
	uint16_t supervision_timeout = timeout * 10; // in ms
	LOG_INF("Connection parameters updated: interval %.2f ms, latency %d intervals, timeout %d ms",
		connection_interval, latency, supervision_timeout);
}
void on_le_phy_updated(struct bt_conn *conn, struct bt_conn_le_phy_info *param)		//更新PHY成功回调函数
{
	if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_1M) {
		LOG_INF("PHY updated. New PHY: 1M");
	} else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_2M) {
		LOG_INF("PHY updated. New PHY: 2M");
	} else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_CODED_S8) {
		LOG_INF("PHY updated. New PHY: Long Range");
	}
}
void on_le_data_len_updated(struct bt_conn *conn, struct bt_conn_le_data_len_info *info)	//更新数据长度成功回调函数
{
	uint16_t tx_len = info->tx_max_len;
	uint16_t tx_time = info->tx_max_time;
	uint16_t rx_len = info->rx_max_len;
	uint16_t rx_time = info->rx_max_time;
	LOG_INF("Data length updated. Length %d/%d bytes, time %d/%d us", tx_len, rx_len, tx_time,
		rx_time);
}
/* 更新连接参数部分代码* end**********************************************************************/


/* 蓝牙广播设置相关部分代码* start*****************************************************************/
static struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(// 静态修改广播扫描参数 
(BT_LE_ADV_OPT_CONNECTABLE |
 BT_LE_ADV_OPT_USE_IDENTITY), //链接广播使用用户自定义地址
BT_GAP_ADV_FAST_INT_MIN_1,    //最小广播间隔30ms = 48 x 0.625ms
BT_GAP_ADV_FAST_INT_MAX_1,    //最大广播间隔60ms = 96 x 0.625ms
NULL);                        //非定向广播设置为NULL
static const struct bt_data ad[]={// 设置广播信息 
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),   //可发现，并且不支持经典蓝牙
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),           //广播名称
};
static const struct bt_data sd[] = {// 设置广播扫描应答 
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),	//应答NUS服务的UUID 128bytes全称
};
void on_connected(struct bt_conn *conn, uint8_t err)//蓝牙连接成功回调函数
{
	if (err) {
		LOG_ERR("Connection error %d", err);
		return;
	}
	LOG_INF("Connected");
	my_conn = bt_conn_ref(conn);
  	LED_BLE_L;	//连接指示灯点亮
 
	struct bt_conn_info info;				//连接参数结构体
	err = bt_conn_get_info(conn, &info);	//获取连接参数信息
	if (err) {
		LOG_ERR("bt_conn_get_info() returned %d", err);
		return;
	}
	
	update_phy(my_conn);
	update_data_length(my_conn);
	update_mtu(my_conn);

}
void on_disconnected(struct bt_conn *conn, uint8_t reason)//蓝牙断开事件回调函数
{
	LOG_INF("Disconnected. Reason %d", reason);
	if (auth_conn) {
		bt_conn_unref(auth_conn);
		auth_conn = NULL;
	}

	if (my_conn) {
		bt_conn_unref(my_conn);
		my_conn = NULL;
		LED_BLE_H;
	}
}
struct bt_conn_cb connection_callbacks = {//申明链接回调结构体
	.connected = on_connected,
	.disconnected = on_disconnected,

};
/* 蓝牙广播设置相关部分代码* end*******************************************************************/





/* 串口透传NUS 服务部分代码* start********************************************************************/
static struct bt_conn_auth_cb conn_auth_callbacks;
static struct bt_conn_auth_info_cb conn_auth_info_callbacks;
static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data, uint16_t len)	//NUS服务收到数据事件-回调函数
{
	int err;
	err = uart_tx(uart_dev, data, len, SYS_FOREVER_MS);
};
static void bt_send_enabled_cb(enum bt_nus_send_status status)	//NUS发送完成回调
{

};
static void  bt_sent_cb(struct bt_conn *conn)	//NUS发送主机事件回调
{

};
static struct bt_nus_cb nus_cb = {	//用于初始化的回调事件结构体
	.received = bt_receive_cb,
	.send_enabled = bt_send_enabled_cb,
	.sent = bt_sent_cb,	
};
/* 串口透传NUS 服务部分代码* end**********************************************************************/



/* USB CDC ACM   部分代码* start********************************************************************/
static void uart_callback(const struct device *dev,  struct uart_event *evt,  void *user_data)	//USB CDC UART 事件回调
{
	struct device *uart = user_data;
	int err;
	buffer_t push_buf; 
	buffer_t pop_buf; 

	uint16_t length;
	switch (evt->type) {
	case UART_TX_DONE:	
		break;
	case UART_TX_ABORTED:
		break;
	case UART_RX_DISABLED:
   		err = uart_rx_enable(uart_dev, rx_buffer[0], sizeof(rx_buffer[0]), -1);		//开启连续接受模式
		break;
	case UART_RX_RDY:
		length = evt->data.rx.len;
		memcpy(&my_data_array[data_cnt], evt->data.rx.buf, length);	//数据存入缓存
		push_buf.length = length;	//数据地址存入堆栈
		push_buf.p_data = &my_data_array[data_cnt];
		err = k_msgq_put(&device_message_queue, &push_buf, K_FOREVER);
		if (err) {
			LOG_ERR("Return value from k_msgq_put = %d", err);
		}

		err = k_msgq_get(&device_message_queue, &pop_buf, K_FOREVER);
		if (err) {
			LOG_ERR("Return value from k_msgq_get = %d", err);
		}
		bt_nus_send(NULL, pop_buf.p_data, pop_buf.length);
		data_cnt = data_cnt + length;
		break;
	case UART_RX_BUF_REQUEST:
		err = uart_rx_buf_rsp(uart, next_buf,
			sizeof(rx_buffer[0]));
		if (err) {
			LOG_WRN("UART RX buf rsp: %d", err);
		}	
		break;
	case UART_RX_BUF_RELEASED:
		break;
	case UART_RX_STOPPED:
		break;
	}	
}
static bool uart_test_async_api(const struct device *dev)
{
	const struct uart_driver_api *api=
		(const struct uart_driver_api *)dev->api;
	return (api->callback_set!=NULL);
}
#if CONFIG_USB_UART_ASYNC_ADAPTER
UART_ASYNC_ADAPTER_INST_DEFINE(async_adapter);
#else
static const struct device *const async_adapter;
#endif
void usb_cdc_acm_init()	//初始化USB CDC UART
{
	int err;
	uint8_t *buf;
	if(!device_is_ready(uart_dev)){
		printk("device %s is not ready; exiting\n", uart_dev->name);
	}
    if (IS_ENABLED(CONFIG_USB_DEVICE_STACK)) {
		err = usb_enable(NULL);
		if (err && (err != -EALREADY)) {
			printk("Failed to enable USB\n");
			return;
		}
	}
    if (IS_ENABLED(CONFIG_USB_UART_ASYNC_ADAPTER) && !uart_test_async_api(uart_dev)) {
		/* Implement API adapter */
		uart_async_adapter_init(async_adapter, uart_dev);
		uart_dev = async_adapter;
	}	
	err = uart_callback_set(uart_dev, uart_callback, (void *)uart_dev);
	__ASSERT(err == 0, "Failed to set callback");
    err = uart_rx_enable(uart_dev, rx_buffer[0], sizeof(rx_buffer)[0], -1);		//中断里面清除阻塞开启连续接受
}
/* USB CDC ACM   部分代码* end**********************************************************************/



/*
  name      ：bt_private_set_addr（）
  function  ：静态修改广播MAC地址
  param     : none
  return    : none
  notice    : 需要在广播初始化之前完成
*/
static void bt_private_set_addr()
{
  int err;
  bt_addr_le_t addr;
	err = bt_addr_le_from_str("FF:EE:DD:CC:BB:AA", "random", &addr);
	if (err) {
		printk("Invalid BT address (err %d)\n", err);
	}

	err = bt_id_create(&addr, NULL);
	if (err < 0) {
		printk("Creating new ID failed (err %d)\n", err);
	}
}



int main(void)
{
    int err;
	int blink_status = 0;  
	usb_cdc_acm_init();
  	Led_Button_init();	
	pow_init();

	
  //bt_private_set_addr();

  /* 注册连接事件回调 */
	bt_conn_cb_register(&connection_callbacks);
  /* 使能协议栈 */
    err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return;
	}
	/* 初始化NUS服务 */
	err = bt_nus_init(&nus_cb);
	if (err) {
		LOG_ERR("Failed to initialize UART service (err: %d)", err);
		return;
	}
  /* 按照设置好的adv_param参数开启广播 */
	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return;
	}
  LOG_INF("Advertising successfully started");

  	k_timer_start(&timer0, K_MSEC(1), K_MSEC(10));

	
	for (;;) {
    	//线程中不能用死循环造成堵塞
		k_sleep(K_MSEC(1000));
	}
}



/* 模拟传感器数据线程 start********************************************************************/
#define STACKSIZE 1024
#define PRIORITY 7
void send_data_thread(void)//新建一个线程，用于模拟传感器发送数据
{
	while (1) {
		k_sleep(K_MSEC(500));
	}
}
//静态创建线程
K_THREAD_DEFINE(send_data_thread_id, STACKSIZE, send_data_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
/* 模拟传感器数据线程 end**********************************************************************/