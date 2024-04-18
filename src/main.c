
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/addr.h>
/*�޸����������Ӳ����õ�*/
#include <zephyr/bluetooth/gatt.h>
/*��������ͷ�ļ�*/
#include <zephyr/bluetooth/conn.h>
/* ����û�MY LBS ���񣨰���+��ƣ�ͷ�ļ�*/
#include "my_lbs.h"


#include "led_button.h"
#include "main.h"


// ע�����logģ�� 
#define LOG_MODULE_NAME Sok_log_init
LOG_MODULE_REGISTER(LOG_MODULE_NAME);
// �Զ���㲥���� 
#define DEVICE_NAME "Sok_nRF5340_slave"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
/*/  �Զ��������豸my_conn
//  Connect management �� bt_conn
//  Zephyr����Э��ջʹ��bt_conn����(*my_conn)�����������豸
//  note1���ýṹ�岻��Ӧ�ó��򹫿�
//  note2������ʹ��bt_conn_git_info()��ȡ������Ϣ
//  note3������ʹ��bt_con_ref()ȷ������ָ�룩��Ч��*/
struct bt_conn *my_conn = NULL;



/* �������Ӳ������ִ���* start********************************************************************/

static void update_phy(struct bt_conn *conn)	//function 1 : ����radio��phy
{
	int err;
	const struct bt_conn_le_phy_param preferred_phy = {	//���ͺͽ��ܵĵ������ʿ��Ե�������
		.options = BT_CONN_LE_PHY_OPT_NONE,
		.pref_rx_phy = BT_GAP_LE_PHY_2M,
		.pref_tx_phy = BT_GAP_LE_PHY_2M,
	};
	err = bt_conn_le_phy_update(conn, &preferred_phy);
	if (err) {
		LOG_ERR("bt_conn_le_phy_update() returned %d", err);
	}
};

static void update_data_length(struct bt_conn *conn)	//function 2 �� �������ݳ���DLE
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
static void update_mtu(struct bt_conn *conn)	//function 3 �� ����MTU
{
	int err;
	exchange_params.func = exchange_func;
	err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (err) {
		LOG_ERR("bt_gatt_exchange_mtu failed (err %d)", err);
	}
}


void on_le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,	//�������ӳɹ��ص�����
			 uint16_t timeout)
{
	double connection_interval = interval * 1.25; // in ms
	uint16_t supervision_timeout = timeout * 10; // in ms
	LOG_INF("Connection parameters updated: interval %.2f ms, latency %d intervals, timeout %d ms",
		connection_interval, latency, supervision_timeout);
}


void on_le_phy_updated(struct bt_conn *conn, struct bt_conn_le_phy_info *param)		//����PHY�ɹ��ص�����
{
	if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_1M) {
		LOG_INF("PHY updated. New PHY: 1M");
	} else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_2M) {
		LOG_INF("PHY updated. New PHY: 2M");
	} else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_CODED_S8) {
		LOG_INF("PHY updated. New PHY: Long Range");
	}
}

void on_le_data_len_updated(struct bt_conn *conn, struct bt_conn_le_data_len_info *info)	//�������ݳ��ȳɹ��ص�����
{
	uint16_t tx_len = info->tx_max_len;
	uint16_t tx_time = info->tx_max_time;
	uint16_t rx_len = info->rx_max_len;
	uint16_t rx_time = info->rx_max_time;
	LOG_INF("Data length updated. Length %d/%d bytes, time %d/%d us", tx_len, rx_len, tx_time,
		rx_time);
}



/* �������Ӳ������ִ���* end**********************************************************************/


/* �����㲥������ز��ִ���* start*****************************************************************/
static struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(// ��̬�޸Ĺ㲥ɨ����� 
(BT_LE_ADV_OPT_CONNECTABLE |
 BT_LE_ADV_OPT_USE_IDENTITY), //���ӹ㲥ʹ���û��Զ����ַ
BT_GAP_ADV_FAST_INT_MIN_1,    //��С�㲥���30ms = 48 x 0.625ms
BT_GAP_ADV_FAST_INT_MAX_1,    //���㲥���60ms = 96 x 0.625ms
NULL);                        //�Ƕ���㲥����ΪNULL
static const struct bt_data ad[]={// ���ù㲥��Ϣ 
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),   //�ɷ��֣����Ҳ�֧�־�������
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),           //�㲥����
};
static const struct bt_data sd[] = {// ���ù㲥ɨ��Ӧ�� 
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),   					//����ɨ��UUID��Ϣ
};
void on_connected(struct bt_conn *conn, uint8_t err)//�������ӳɹ��ص�����
{
	if (err) {
		LOG_ERR("Connection error %d", err);
		return;
	}
	LOG_INF("Connected");
	my_conn = bt_conn_ref(conn);
  	LED_BLE_L;	//����ָʾ�Ƶ���

	struct bt_conn_info info;				//���Ӳ����ṹ��
	err = bt_conn_get_info(conn, &info);	//��ȡ���Ӳ�����Ϣ
	if (err) {
		LOG_ERR("bt_conn_get_info() returned %d", err);
		return;
	}
	/*��ӡLOG���µ�ǰ�����Ӳ���*/
	double connection_interval = info.le.interval * 1.25; 	// ���Ӽ��unit��ms
	uint16_t supervision_timeout = info.le.timeout * 10;	// ���ӳ�ʱunit��ms
	LOG_INF("Connection parameters: interval %.2f ms, latency %d intervals, timeout %d ms",
		connection_interval, info.le.latency, supervision_timeout);
	
	update_phy(my_conn);
	update_data_length(my_conn);
	update_mtu(my_conn);

}
void on_disconnected(struct bt_conn *conn, uint8_t reason)//�����Ͽ��¼��ص�����
{
	LOG_INF("Disconnected. Reason %d", reason);
	bt_conn_unref(my_conn);
  	LED_BLE_H;	//Ϩ������ָʾ��
}
struct bt_conn_cb connection_callbacks = {//�������ӻص��ṹ��
	.connected = on_connected,
	.disconnected = on_disconnected,
	/* �����������Ӳ����ص��¼� */
	.le_param_updated = on_le_param_updated,
	.le_phy_updated = on_le_phy_updated,
	.le_data_len_updated = on_le_data_len_updated,	
};
/* �����㲥������ز��ִ���* end*******************************************************************/





/* ���LSB ���񲿷ִ���* start********************************************************************/
static bool app_button_state;
static void app_led_cb(bool led_state)//LED�����·��ص�����
{
	if(led_state == true)
	LED_MARK_L;
	else
	LED_MARK_H;
};
static bool app_button_cb(void)//button������ȡ�ص���������notify�����Ի����¼��ص���
{
	return app_button_state;
};
static struct my_lbs_cb app_callbacks = {//���ڳ�ʼ���ص��¼������Ľṹ������
	.led_cb = app_led_cb,
	.button_cb = app_button_cb,
};
/* ���LSB ���񲿷ִ���* end**********************************************************************/



/*
  name      ��bt_private_set_addr����
  function  ����̬�޸Ĺ㲥MAC��ַ
  param     : none
  return    : none
  notice    : ��Ҫ�ڹ㲥��ʼ��֮ǰ���
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

  /* ע�������¼��ص� */
	bt_conn_cb_register(&connection_callbacks);
  /* ʹ��Э��ջ */
    err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return;
	}
	/* ���LBS�����ʼ�� */
	err = my_lbs_init(&app_callbacks);
	if (err) {
		printk("Failed to init LBS (err:%d)\n", err);
		return;
	}	
  /* �������úõ�adv_param���������㲥 */
	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return;
	}
  LOG_INF("Advertising successfully started");
	for (;;) {
    	//�߳��в�������ѭ����ɶ���
		k_sleep(K_MSEC(1000));
	}
}



/* ģ�⴫���������߳� start********************************************************************/
#define STACKSIZE 1024
#define PRIORITY 7
static uint32_t app_sensor_value = 100;
void send_data_thread(void)//�½�һ���̣߳�����ģ�⴫������������
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
//��̬�����߳�
K_THREAD_DEFINE(send_data_thread_id, STACKSIZE, send_data_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
/* ģ�⴫���������߳� end**********************************************************************/