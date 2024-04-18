
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/addr.h>
/*修改数据与连接参数用到*/
#include <zephyr/bluetooth/gatt.h>
/*蓝牙链接头文件*/
#include <zephyr/bluetooth/conn.h>
/* 添加用户MY LBS 服务（按键+点灯）头文件*/
#include "my_lbs.h"


#include "led_button.h"
#include "main.h"


// 注册加载log模块 
#define LOG_MODULE_NAME Sok_log_init
LOG_MODULE_REGISTER(LOG_MODULE_NAME);
// 自定义广播名称 
#define DEVICE_NAME "Sok_nRF5340_slave"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
/*/  自定义链接设备my_conn
//  Connect management ： bt_conn
//  Zephyr蓝牙协议栈使用bt_conn抽象(*my_conn)来连接其他设备
//  note1：该结构体不向应用程序公开
//  note2：可以使用bt_conn_git_info()获取有限信息
//  note3：可以使用bt_con_ref()确保对象（指针）有效性*/
struct bt_conn *my_conn = NULL;



/* 更新连接参数部分代码* start********************************************************************/

static void update_phy(struct bt_conn *conn)	//function 1 : 更新radio的phy
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

static void update_data_length(struct bt_conn *conn)	//function 2 ： 更新数据长度DLE
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
static void update_mtu(struct bt_conn *conn)	//function 3 ： 更新MTU
{
	int err;
	exchange_params.func = exchange_func;
	err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (err) {
		LOG_ERR("bt_gatt_exchange_mtu failed (err %d)", err);
	}
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
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),   					//返回扫描UUID信息
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
	/*打印LOG看下当前的连接参数*/
	double connection_interval = info.le.interval * 1.25; 	// 连接间隔unit：ms
	uint16_t supervision_timeout = info.le.timeout * 10;	// 连接超时unit：ms
	LOG_INF("Connection parameters: interval %.2f ms, latency %d intervals, timeout %d ms",
		connection_interval, info.le.latency, supervision_timeout);
	
	update_phy(my_conn);
	update_data_length(my_conn);
	update_mtu(my_conn);

}
void on_disconnected(struct bt_conn *conn, uint8_t reason)//蓝牙断开事件回调函数
{
	LOG_INF("Disconnected. Reason %d", reason);
	bt_conn_unref(my_conn);
  	LED_BLE_H;	//熄灭连接指示灯
}
struct bt_conn_cb connection_callbacks = {//申明链接回调结构体
	.connected = on_connected,
	.disconnected = on_disconnected,
	/* 新增更新连接参数回调事件 */
	.le_param_updated = on_le_param_updated,
	.le_phy_updated = on_le_phy_updated,
	.le_data_len_updated = on_le_data_len_updated,	
};
/* 蓝牙广播设置相关部分代码* end*******************************************************************/





/* 点灯LSB 服务部分代码* start********************************************************************/
static bool app_button_state;
static void app_led_cb(bool led_state)//LED特征下发回调函数
{
	if(led_state == true)
	LED_MARK_L;
	else
	LED_MARK_H;
};
static bool app_button_cb(void)//button特征读取回调函数（无notify，所以会有事件回调）
{
	return app_button_state;
};
static struct my_lbs_cb app_callbacks = {//用于初始化回调事件函数的结构体声明
	.led_cb = app_led_cb,
	.button_cb = app_button_cb,
};
/* 点灯LSB 服务部分代码* end**********************************************************************/



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
  Led_Button_init();
  //bt_private_set_addr();

  /* 注册连接事件回调 */
	bt_conn_cb_register(&connection_callbacks);
  /* 使能协议栈 */
    err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return;
	}
	/* 添加LBS服务初始化 */
	err = my_lbs_init(&app_callbacks);
	if (err) {
		printk("Failed to init LBS (err:%d)\n", err);
		return;
	}	
  /* 按照设置好的adv_param参数开启广播 */
	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return;
	}
  LOG_INF("Advertising successfully started");
	for (;;) {
    	//线程中不能用死循环造成堵塞
		k_sleep(K_MSEC(1000));
	}
}



/* 模拟传感器数据线程 start********************************************************************/
#define STACKSIZE 1024
#define PRIORITY 7
static uint32_t app_sensor_value = 100;
void send_data_thread(void)//新建一个线程，用于模拟传感器发送数据
{
	while (1) {
		app_sensor_value++;
		if (app_sensor_value == 200) {
			app_sensor_value = 100;
		}
		my_lbs_send_sensor_notify(app_sensor_value);
		k_sleep(K_MSEC(500));
	}
}
//静态创建线程
K_THREAD_DEFINE(send_data_thread_id, STACKSIZE, send_data_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
/* 模拟传感器数据线程 end**********************************************************************/