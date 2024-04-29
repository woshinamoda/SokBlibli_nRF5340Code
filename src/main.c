
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/addr.h>
/*�޸����������Ӳ����õ�*/
#include <zephyr/bluetooth/gatt.h>
/*��������ͷ�ļ�*/
#include <zephyr/bluetooth/conn.h>
/*���nus����ͷ�ļ�*/
#include <bluetooth/services/nus.h>
/*��� USB CDC ADCͷ�ļ�*/
#include <zephyr/usb/usb_device.h>
#include "uart_async_adapter.h"


#include "led_button.h"
#include "main.h"


// ע�����logģ�� 
#define LOG_MODULE_NAME Sok_log_init
LOG_MODULE_REGISTER(LOG_MODULE_NAME);
// �Զ���㲥���� 
#define DEVICE_NAME "Sok_NUS_Service_Test"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
/*  �Զ��������豸my_conn
//  Connect management �� bt_conn
//  Zephyr����Э��ջʹ��bt_conn����(*my_conn)�����������豸
//  note1���ýṹ�岻��Ӧ�ó��򹫿�
//  note2������ʹ��bt_conn_git_info()��ȡ������Ϣ
//  note3������ʹ��bt_con_ref()ȷ������ָ�룩��Ч��*/
struct bt_conn *my_conn = NULL;		//�û����ӳ���
struct bt_conn *auth_conn = NULL;	//�Զ���������
// USB CDC ACM UART ����
#define DATA_SIZE_MAX  230			//���ڣ��򴫸�����һ������յ�230bytes����
/* ���ݽڵ㣨zephyr��cdc_acm_uart����ȡ���� */
static const struct device *uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);	
typedef struct			//USB�յ����ݣ�ָ� &&  ������׼�������ݣ� ���浽��FIFO�У�ͨ��NUS�����ͳ�ȥ																	
{
	uint8_t * p_data;	//ʵ�ʴ�����ݵ�����								
	uint16_t length;	//�������ݳ���
} buffer_t;
uint8_t rx_buffer[2][DATA_SIZE_MAX];
uint8_t *next_buf = rx_buffer[1];
uint8_t my_data_array[50000];	//����USBָ�� && �������������� eg��50000bytes
uint16_t data_cnt;
K_MSGQ_DEFINE(device_message_queue, sizeof(buffer_t), 200, 2);	//��̬�������У����ƣ�device_message_queue����Сbutter_t��������Ϣ����200����ÿ��Ԫ�ض���2bytes


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


/* �������Ӳ������ִ���* start********************************************************************/
static void update_phy(struct bt_conn *conn)	//function 1 : ��̬����radio��phy
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
static void update_data_length(struct bt_conn *conn)	//function 2 �� ��̬�������ݳ���DLE
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
static void update_mtu(struct bt_conn *conn)	//function 3 �� ��̬����MTU
{
	int err;
	exchange_params.func = exchange_func;
	err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (err) {
		LOG_ERR("bt_gatt_exchange_mtu failed (err %d)", err);
	}
}
static void update_led_param(struct bt_conn *conn)	//function 4: ��̬�������Ӳ���
{
	struct bt_le_conn_param param;
	param.interval_max = 800;
	param.interval_min = 800;
	param.latency = 0;
	param.timeout = 400;
	bt_conn_le_param_update(conn, &param);
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
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),	//Ӧ��NUS�����UUID 128bytesȫ��
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
	
	update_phy(my_conn);
	update_data_length(my_conn);
	update_mtu(my_conn);

}
void on_disconnected(struct bt_conn *conn, uint8_t reason)//�����Ͽ��¼��ص�����
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
struct bt_conn_cb connection_callbacks = {//�������ӻص��ṹ��
	.connected = on_connected,
	.disconnected = on_disconnected,

};
/* �����㲥������ز��ִ���* end*******************************************************************/





/* ����͸��NUS ���񲿷ִ���* start********************************************************************/
static struct bt_conn_auth_cb conn_auth_callbacks;
static struct bt_conn_auth_info_cb conn_auth_info_callbacks;
static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data, uint16_t len)	//NUS�����յ������¼�-�ص�����
{
	int err;
	err = uart_tx(uart_dev, data, len, SYS_FOREVER_MS);
};
static void bt_send_enabled_cb(enum bt_nus_send_status status)	//NUS������ɻص�
{

};
static void  bt_sent_cb(struct bt_conn *conn)	//NUS���������¼��ص�
{

};
static struct bt_nus_cb nus_cb = {	//���ڳ�ʼ���Ļص��¼��ṹ��
	.received = bt_receive_cb,
	.send_enabled = bt_send_enabled_cb,
	.sent = bt_sent_cb,	
};
/* ����͸��NUS ���񲿷ִ���* end**********************************************************************/



/* USB CDC ACM   ���ִ���* start********************************************************************/
static void uart_callback(const struct device *dev,  struct uart_event *evt,  void *user_data)	//USB CDC UART �¼��ص�
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
   		err = uart_rx_enable(uart_dev, rx_buffer[0], sizeof(rx_buffer[0]), -1);		//������������ģʽ
		break;
	case UART_RX_RDY:
		length = evt->data.rx.len;
		memcpy(&my_data_array[data_cnt], evt->data.rx.buf, length);	//���ݴ��뻺��
		push_buf.length = length;	//���ݵ�ַ�����ջ
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
void usb_cdc_acm_init()	//��ʼ��USB CDC UART
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
    err = uart_rx_enable(uart_dev, rx_buffer[0], sizeof(rx_buffer)[0], -1);		//�ж������������������������
}
/* USB CDC ACM   ���ִ���* end**********************************************************************/



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
	usb_cdc_acm_init();
  	Led_Button_init();	
	pow_init();

	
  //bt_private_set_addr();

  /* ע�������¼��ص� */
	bt_conn_cb_register(&connection_callbacks);
  /* ʹ��Э��ջ */
    err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return;
	}
	/* ��ʼ��NUS���� */
	err = bt_nus_init(&nus_cb);
	if (err) {
		LOG_ERR("Failed to initialize UART service (err: %d)", err);
		return;
	}
  /* �������úõ�adv_param���������㲥 */
	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return;
	}
  LOG_INF("Advertising successfully started");

  	k_timer_start(&timer0, K_MSEC(1), K_MSEC(10));

	
	for (;;) {
    	//�߳��в�������ѭ����ɶ���
		k_sleep(K_MSEC(1000));
	}
}



/* ģ�⴫���������߳� start********************************************************************/
#define STACKSIZE 1024
#define PRIORITY 7
void send_data_thread(void)//�½�һ���̣߳�����ģ�⴫������������
{
	while (1) {
		k_sleep(K_MSEC(500));
	}
}
//��̬�����߳�
K_THREAD_DEFINE(send_data_thread_id, STACKSIZE, send_data_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
/* ģ�⴫���������߳� end**********************************************************************/