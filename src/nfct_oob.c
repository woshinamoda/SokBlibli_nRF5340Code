#include "nfct_oob.h"
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <dk_buttons_and_leds.h>

#include <nfc_t4t_lib.h>
#include <nfc/t4t/ndef_file.h>

#include <nfc/ndef/msg.h>
#include <nfc/ndef/ch.h>
#include <nfc/ndef/ch_msg.h>
#include <nfc/ndef/le_oob_rec.h>
#include <nfc/ndef/le_oob_rec_parser.h>
#include <nfc/ndef/text_rec.h>

#include <nfc/tnep/tag.h>
#include <nfc/tnep/ch.h>
#include "led_button.h"

#define TNEP_INITIAL_MSG_RECORD_COUNT 1
#define NDEF_TNEP_MSG_SIZE 1024 //nordic的NFC标签可以存储1024bytes

#define NFC_NDEF_LE_OOB_REC_PARSER_BUFF_SIZE 150
#define NFC_TNEP_BUFFER_SIZE 1024

#define NDEF_MSG_BUF_SIZE 256
#define AUTH_SC_FLAG 0x08

uint8_t eCon_device_name[] = "eConAlpha";
static const uint8_t en_code[] = {'e', 'n'};
static struct bt_le_oob oob_local;
static uint8_t tk_value[NFC_NDEF_LE_OOB_REC_TK_LEN];
static uint8_t remote_tk_value[NFC_NDEF_LE_OOB_REC_TK_LEN];
/*
state:use NDEF message buffer must have two TX buffer pointer
1.用于标记NDEF消息
2.编码新的NDEF消息供以后使用
*/
static uint8_t tag_buffer[NDEF_TNEP_MSG_SIZE];
static uint8_t tag_buffer2[NDEF_TNEP_MSG_SIZE];
static struct k_poll_event events[NFC_TNEP_EVENTS_NUMBER];  //poller事件
static struct bt_le_oob oob_remote;
static bool use_remote_tk;
static struct k_poll_signal pair_signal;

extern char user_addr_str[BT_ADDR_STR_LEN];

/*pair key perpare begin*************************************************************************************************************/
static int tk_value_generate(void)
{
	int err;
	memcmp(tk_value,&user_addr_str[2],16);
	printk("tk value is: %s\n", tk_value);

	return err;
}
static void pair_key_generate_init(void)
{
	k_poll_signal_init(&pair_signal);
	k_poll_event_init(&events[NFC_TNEP_EVENTS_NUMBER],
			  K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
			  &pair_signal);
}
static int paring_key_generate(void)
{
	int err;

	printk("Generating new pairing keys\n");

	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob_local);
	if (err) {
		printk("Error while fetching local OOB data: %d\n", err);
	}

	return tk_value_generate();
}
static void paring_key_process(void)
{
	int err;

	if (events[NFC_TNEP_EVENTS_NUMBER].state == K_POLL_STATE_SIGNALED) {
		err = paring_key_generate();
		if (err) {
			printk("Pairing key generation error: %d\n", err);
		}

		k_poll_signal_reset(events[NFC_TNEP_EVENTS_NUMBER].signal);
		events[NFC_TNEP_EVENTS_NUMBER].state = K_POLL_STATE_NOT_READY;
	}
}
/*pair key perpare end***************************************************************************************************************/




/*srcurity conn start***************************************************************************************************************/
static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}
static void lesc_oob_data_set(struct bt_conn *conn,struct bt_conn_oob_info *oob_info)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_conn_info info;

	err = bt_conn_get_info(conn, &info);
	if (err) {
		return;
	}

	struct bt_le_oob_sc_data *oob_data_local =
		oob_info->lesc.oob_config != BT_CONN_OOB_REMOTE_ONLY
					  ? &oob_local.le_sc_data
					  : NULL;
	struct bt_le_oob_sc_data *oob_data_remote =
		oob_info->lesc.oob_config != BT_CONN_OOB_LOCAL_ONLY
					  ? &oob_remote.le_sc_data
					  : NULL;

	if (oob_data_remote &&
	    bt_addr_le_cmp(info.le.remote, &oob_remote.addr)) {
		bt_addr_le_to_str(info.le.remote, addr, sizeof(addr));
		printk("No OOB data available for remote %s", addr);
		bt_conn_auth_cancel(conn);
		return;
	}

	if (oob_data_local &&
	    bt_addr_le_cmp(info.le.local, &oob_local.addr)) {
		bt_addr_le_to_str(info.le.local, addr, sizeof(addr));
		printk("No OOB data available for local %s", addr);
		bt_conn_auth_cancel(conn);
		return;
	}

	err = bt_le_oob_set_sc_data(conn, oob_data_local, oob_data_remote);
	if (err) {
		printk("Error while setting OOB data: %d\n", err);
	}
}
static void legacy_tk_value_set(struct bt_conn *conn)
{
	int err;
	const uint8_t *tk = use_remote_tk ? remote_tk_value : tk_value;

	err = bt_le_oob_set_legacy_tk(conn, tk);
	if (err) {
		printk("TK value set error: %d\n", err);
	}

	use_remote_tk = false;
}
static void auth_oob_data_request(struct bt_conn *conn,  struct bt_conn_oob_info *info)
{
	if (info->type == BT_CONN_OOB_LE_SC) {
		printk("LESC OOB data requested\n");
		lesc_oob_data_set(conn, info);
	}

	if (info->type == BT_CONN_OOB_LE_LEGACY) {
		printk("Legacy TK value requested\n");
		legacy_tk_value_set(conn);
	}
}
static enum bt_security_err pairing_accept(struct bt_conn *conn,const struct bt_conn_pairing_feat *const feat)
{
	if (feat->oob_data_flag && (!(feat->auth_req & AUTH_SC_FLAG))) {
		bt_le_oob_set_legacy_flag(true);
	}

	return BT_SECURITY_ERR_SUCCESS;

}
static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing completed: %s, bonded: %d\n", addr, bonded);

	k_poll_signal_raise(&pair_signal, 0);
	bt_le_oob_set_sc_flag(false);
	bt_le_oob_set_legacy_flag(false);
}
static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing failed conn: %s, reason %d\n", addr, reason);

	k_poll_signal_raise(&pair_signal, 0);
	bt_le_oob_set_sc_flag(false);
	bt_le_oob_set_legacy_flag(false);
}
//具有安全配对中 - - - 身份验证的回调事件结构体
static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,							//取消正在进行的用户请求
	.oob_data_request = auth_oob_data_request,		//请求用户提供‘带外’数据
	.pairing_accept = pairing_accept,				//查询是否继续输入配对
};
//具有安全配对中 - - - 经过认证的配对信息回调结构体
static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,			//配对完成回调事件
	.pairing_failed = pairing_failed				//配对失败
};
/*srcurity conn end*****************************************************************************************************************/

/*tnep tag oob start****************************************************************************************************************/
static int check_oob_carrier(const struct nfc_tnep_ch_record *ch_record, const struct nfc_ndef_record_desc **oob_data)
{
	const struct nfc_ndef_ch_ac_rec *ac_rec = NULL;

	for (size_t i = 0; i < ch_record->count; i++) {
		if (nfc_ndef_le_oob_rec_check(ch_record->carrier[i])) {
			*oob_data = ch_record->carrier[i];
		}
	}

	if (!oob_data) {
		printk("Connection Handover Requester not supporting OOB BLE\n");
		return -EINVAL;
	}

	/* Look for the corresponding Alternative Carrier Record. */
	for (size_t i = 0; i < ch_record->count; i++) {
		if (((*oob_data)->id_length == ch_record->ac[i].carrier_data_ref.length) &&
		    (memcmp((*oob_data)->id,
			    ch_record->ac[i].carrier_data_ref.data,
			    (*oob_data)->id_length) == 0)) {
			ac_rec = &ch_record->ac[i];
		}
	}

	if (!ac_rec) {
		printk("No Alternative Carrier Record for OOB LE carrier\n");
		return -EINVAL;
	}

	/* Check carrier state */
	if ((ac_rec->cps != NFC_AC_CPS_ACTIVE) &&
	    (ac_rec->cps != NFC_AC_CPS_ACTIVATING)) {
		printk("LE OBB Carrier inactive\n");
		return -EINVAL;
	}

	return 0;
}
static int oob_le_data_handle(const struct nfc_ndef_record_desc *rec, bool request)
{
	int err;
	const struct nfc_ndef_le_oob_rec_payload_desc *oob;
	uint8_t desc_buf[NFC_NDEF_LE_OOB_REC_PARSER_BUFF_SIZE];
	uint32_t desc_buf_len = sizeof(desc_buf);

	err = nfc_ndef_le_oob_rec_parse(rec, desc_buf,
					&desc_buf_len);
	if (err) {
		printk("Error during NDEF LE OOB Record parsing, err: %d.\n",
			err);
	}

	oob = (struct nfc_ndef_le_oob_rec_payload_desc *) desc_buf;

	nfc_ndef_le_oob_rec_printout(oob);

	if ((*oob->le_role != NFC_NDEF_LE_OOB_REC_LE_ROLE_CENTRAL_ONLY) &&
	    (*oob->le_role != NFC_NDEF_LE_OOB_REC_LE_ROLE_CENTRAL_PREFFERED)) {
		printk("Unsupported Device LE Role\n");
		return -EINVAL;
	}

	if (oob->le_sc_data) {
		bt_le_oob_set_sc_flag(true);
		oob_remote.le_sc_data = *oob->le_sc_data;
		bt_addr_le_copy(&oob_remote.addr, oob->addr);
	}

	if (oob->tk_value) {
		bt_le_oob_set_legacy_flag(true);
		memcpy(remote_tk_value, oob->tk_value, sizeof(remote_tk_value));
		use_remote_tk = request;
	}



	return 0;
}
static int carrier_prepare(void)
{
	static struct nfc_ndef_le_oob_rec_payload_desc rec_payload;

	NFC_NDEF_LE_OOB_RECORD_DESC_DEF(oob_rec, '0', &rec_payload);
	NFC_NDEF_CH_AC_RECORD_DESC_DEF(oob_ac, NFC_AC_CPS_ACTIVE, 1, "0", 0);

	memset(&rec_payload, 0, sizeof(rec_payload));

	rec_payload.addr = &oob_local.addr;
	rec_payload.le_sc_data = &oob_local.le_sc_data;
	rec_payload.tk_value = tk_value;
	rec_payload.local_name = bt_get_name();
	rec_payload.le_role = NFC_NDEF_LE_OOB_REC_LE_ROLE(
		NFC_NDEF_LE_OOB_REC_LE_ROLE_PERIPH_ONLY);
	rec_payload.appearance = NFC_NDEF_LE_OOB_REC_APPEARANCE(
		CONFIG_BT_DEVICE_APPEARANCE);
	rec_payload.flags = NFC_NDEF_LE_OOB_REC_FLAGS(BT_LE_AD_NO_BREDR);

	return nfc_tnep_ch_carrier_set(&NFC_NDEF_CH_AC_RECORD_DESC(oob_ac),
				       &NFC_NDEF_LE_OOB_RECORD_DESC(oob_rec),
				       1);
}
static int tnep_ch_request_received(const struct nfc_tnep_ch_request *ch_req)
{
	int err;
	const struct nfc_ndef_record_desc *oob_data = NULL;

	if (!ch_req->ch_record.count) {
		return -EINVAL;
	}

	err = check_oob_carrier(&ch_req->ch_record, &oob_data);
	if (err) {
		return err;
	}

	bt_le_adv_stop();

	err = oob_le_data_handle(oob_data, true);
	if (err) {
		return err;
	}

	return carrier_prepare();
}
static struct nfc_tnep_ch_cb ch_cb = {					//定义tnep连接切换服务事件结构体
#if defined(CONFIG_NFC_TAG_CH_REQUESTER)				//默认select角色
	.request_msg_prepare = tnep_ch_request_prepare,
	.select_msg_recv = tnep_ch_select_received,
#endif
	.request_msg_recv = tnep_ch_request_received		//收到连接请求回调事件
};
static int tnep_initial_msg_encode(struct nfc_ndef_msg_desc *msg)
{
	int err;
	struct nfc_ndef_ch_msg_records ch_records;
	static struct nfc_ndef_le_oob_rec_payload_desc rec_payload;
	NFC_NDEF_LE_OOB_RECORD_DESC_DEF(oob_rec, '0', &rec_payload);
	NFC_NDEF_CH_AC_RECORD_DESC_DEF(oob_ac, NFC_AC_CPS_ACTIVE, 1, "0", 0);
	NFC_NDEF_CH_HS_RECORD_DESC_DEF(hs_rec, NFC_NDEF_CH_MSG_MAJOR_VER,
				       NFC_NDEF_CH_MSG_MINOR_VER, 1);

	memset(&rec_payload, 0, sizeof(rec_payload));

	rec_payload.addr = &oob_local.addr;
	rec_payload.le_sc_data = &oob_local.le_sc_data;
	rec_payload.tk_value = tk_value;
	rec_payload.local_name = &eCon_device_name;
	rec_payload.le_role = NFC_NDEF_LE_OOB_REC_LE_ROLE(
		NFC_NDEF_LE_OOB_REC_LE_ROLE_PERIPH_ONLY);
	rec_payload.appearance = NFC_NDEF_LE_OOB_REC_APPEARANCE(
		CONFIG_BT_DEVICE_APPEARANCE);
	rec_payload.flags = NFC_NDEF_LE_OOB_REC_FLAGS(BT_LE_AD_NO_BREDR);

	ch_records.ac = &NFC_NDEF_CH_AC_RECORD_DESC(oob_ac);
	ch_records.carrier = &NFC_NDEF_LE_OOB_RECORD_DESC(oob_rec);
	ch_records.cnt = 1;

	err =  nfc_ndef_ch_msg_hs_create(msg,
					 &NFC_NDEF_CH_RECORD_DESC(hs_rec),
					 &ch_records);
	if (err) {
		return err;
	}
	/*
	function：初始化TNRP时对NDEF进行编码，必须与initial_msg_encode_t回调函数结合使用，在主函数nfc_tnep_tag_initial_msg_create中会作为回调函数嵌入
	@param 1：NDEF message描述符指针
	@param 2：message中的第一条reocrd记录指针，没有就NULL
	@param 3：record数量
	*/
	return nfc_tnep_initial_msg_encode(msg, NULL, 0);

    const uint8_t text[] = "eConAlpha OOB message";
    NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_text, UTF_8, en_code,
                        sizeof(en_code), text,
                        strlen(text));

    return nfc_tnep_initial_msg_encode(msg,
                        &NFC_NDEF_TEXT_RECORD_DESC(nfc_text),
                        1);
}
static void  nfc_callback(void *context, nfc_t4t_event_t event, const uint8_t *data, size_t data_length, uint32_t flags)
{
    switch(event){
        /*读卡器写入NDEF数据长度信息给tag*/
	case NFC_T4T_EVENT_NDEF_UPDATED:    
        if(data_length > 0){
            nfc_tnep_tag_rx_msg_indicate(nfc_t4t_ndef_file_msg_get(data), data_length);
        }
        break;
	case NFC_T4T_EVENT_FIELD_ON:    //检测到poller
        LED_NFC_L;
        break;
	case NFC_T4T_EVENT_FIELD_OFF:   //poller离开NFC
        LED_NFC_H;
        nfc_tnep_tag_on_selected();
        break;

    default:
        break;
    }
}
/*tnep tag oob end*****************************************************************************************************************/

/*running main function start*******************************************************************************************************/
void sercurity_conn_init_main()
{
	int err;
	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		printk("Failed to register authorization callbacks.\n");
		return 0;
	}
	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		printk("Failed to register authorization info callbacks.\n");
		return 0;
	}
}
void nfc_tnep_ndef_init_main()
{
    int err;

	err = paring_key_generate();
	if (err) {
		return 0;
	}

	pair_key_generate_init();

    /* TNEP init */
    err = nfc_tnep_tag_tx_msg_buffer_register(tag_buffer, tag_buffer2, sizeof(tag_buffer));
    if(err){
        printk("cannot register tnep buffer , err: %d\n",err);
        return 0;
    }

    /* 启用TNEP协议通信 
    @param1：TNEP tag 实践
    @param2：事件计数
    @param3：设置NFC TNEP的NDEF数据缓存
    */
    err = nfc_tnep_tag_init(events, ARRAY_SIZE(events), nfc_t4t_ndef_rwpayload_set);
    if(err){
        printk("cannot initialize TNEP protocol, err:%d\n", err);
        return 0;
    }

    /* 创建TYPE 4 tag*/
    err = nfc_t4t_setup(nfc_callback, NULL);
	if (err) {
		printk("NFC T4T setup err: %d\n", err);
		return 0;
	}

    /* 创建初始化 TNEP NDEF message */
    err = nfc_tnep_tag_initial_msg_create(3,    //可选NDEF record最大数
                                        tnep_initial_msg_encode);           //初始化TNEP NDEF 的回调函数
	if (err) {
		printk("Cannot create initial TNEP message, err: %d\n", err);
	}

	/* 初始化TNEP连接切换服务 */
	err = nfc_tnep_ch_service_init(&ch_cb);
	if (err) {
		printk("TNEP CH Service init error: %d\n", err);
		return;
	}


    /* 激活NFC fronted*/
    err = nfc_t4t_emulation_start();
	if (err) {
		printk("NFC T4T emulation start err: %d\n", err);
		return 0;
	}
}
void nfc_tnep_tag_event_handle()
{
    k_poll(events, ARRAY_SIZE(events), K_FOREVER);
    nfc_tnep_tag_process();   
	paring_key_process();
}
/*running main function end*********************************************************************************************************/


