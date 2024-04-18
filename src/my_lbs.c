/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief LED Button Service (LBS) sample
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "my_lbs.h"

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(Lesson4_Exercise1);


static bool indicate_enabled;			//按键cccd描述符状态
static bool notify_mysensor_enabled;	//传感器cccd描述符状态
static bool button_state;				//按键状态


static struct my_lbs_cb lbs_cb;
static struct bt_gatt_indicate_params ind_params;		//定义GATT指示参数

//客户端更新(按键特征)描述符值
static void mylbsbc_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	indicate_enabled = (value == BT_GATT_CCC_INDICATE);
}

//客户端更新(C传感器特征)描述符值
static void mylbsbc_ccc_mysensor_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	notify_mysensor_enabled = (value == BT_GATT_CCC_NOTIFY);
}

//远程设备（客户端）确认指示（使能cccd描述符）的回调函数，在log中打印是否使能成功
static void indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	LOG_DBG("Indication %s\n", err != 0U ? "fail" : "success");
}




//下发点灯指示事件
static ssize_t write_led(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			 uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Attribute write, handle: %u, conn: %p", attr->handle, (void *)conn);

	if (len != 1U) {
		LOG_DBG("Write led: Incorrect data length");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (offset != 0) {
		LOG_DBG("Write led: Incorrect data offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (lbs_cb.led_cb) {

		uint8_t val = *((uint8_t *)buf);

		if (val == 0x00 || val == 0x01) {

			lbs_cb.led_cb(val ? true : false);
		} else {
			LOG_DBG("Write led: Incorrect value");
			return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
		}
	}

	return len;
}

//读取按键状态回调事件
static ssize_t read_button(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			   uint16_t len, uint16_t offset)
{

	const char *value = attr->user_data;

	LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle, (void *)conn);

	if (lbs_cb.button_cb) {	//发生客户端下发读取按键状态事件

		button_state = lbs_cb.button_cb();	//（main.c里面的值传递给button_state）
		return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));	//客户端读取按键状态后回传给服务端，下载历程按着按键不放，点击下发箭头会读取到press变化
	}

	return 0;
}

/* 自定义传感器的uuid */
#define BT_UUID_LBS_MYSENSOR_VAL                                                                   \
	BT_UUID_128_ENCODE(0x00001526, 0x1212, 0xefde, 0x1523, 0x785feabcd123)
#define BT_UUID_LBS_MYSENSOR BT_UUID_DECLARE_128(BT_UUID_LBS_MYSENSOR_VAL)

//静态创建点灯按键服务
BT_GATT_SERVICE_DEFINE(my_lbs_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_LBS),
			//添加按键特征 ***_props = BT_GATT_CHRC_READ | BT_GATT_CHRC_INDICATE [[表示特征属性是： 读属性， 允许带有特征值]]
		    BT_GATT_CHARACTERISTIC(BT_UUID_LBS_BUTTON, BT_GATT_CHRC_READ|BT_GATT_CHRC_INDICATE, BT_GATT_PERM_READ, read_button, NULL, &button_state),
			//添加按键特征值的描述符
			BT_GATT_CCC(mylbsbc_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
			//添加点灯服务
		    BT_GATT_CHARACTERISTIC(BT_UUID_LBS_LED, BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE, NULL, write_led, NULL),
			/*在创建一个用户传感器特征  ******_props = BT_GATT_CHRC_NOTIFY [[允许带有特征值，但不能设置]]*/
			BT_GATT_CHARACTERISTIC(BT_UUID_LBS_MYSENSOR, BT_GATT_CHRC_NOTIFY, BT_GATT_PERM_NONE, NULL,
			       NULL, NULL),
			/*添加传感器特征值的描述符*/
			BT_GATT_CCC(mylbsbc_ccc_mysensor_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);
//初始化赋值事件回调函数
int my_lbs_init(struct my_lbs_cb *callbacks)
{
	if (callbacks) {
		lbs_cb.led_cb = callbacks->led_cb;
		lbs_cb.button_cb = callbacks->button_cb;
	}

	return 0;
}


int my_lbs_send_button_state_indicate(bool button_state)
{
	if (!indicate_enabled) {
		return -EACCES;
	}
	ind_params.attr = &my_lbs_svc.attrs[2];
	ind_params.func = indicate_cb; 
	ind_params.destroy = NULL;
	ind_params.data = &button_state;
	ind_params.len = sizeof(button_state);
	return bt_gatt_indicate(NULL, &ind_params);
}

//传感器服务发送通知函数
//host传递网络核，直接发送bt_gatt_notify
int my_lbs_send_sensor_notify(uint32_t sensor_value)
{
	if (!notify_mysensor_enabled) {
		return -EACCES;
	}

	return bt_gatt_notify(NULL, &my_lbs_svc.attrs[7], &sensor_value, sizeof(sensor_value));
}
