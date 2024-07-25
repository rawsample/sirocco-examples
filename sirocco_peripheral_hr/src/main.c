/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/services/hrs.h>
#include <zephyr/bluetooth/controller.h>

#include <bluetooth/services/lbs.h>

#include <dk_buttons_and_leds.h>

#include <zephyr/bluetooth/sirocco.h>


#define RUN_STATUS_LED	DK_LED1
#define CON_STATUS_LED	DK_LED2
#define USER_LED        DK_LED3
#define USER_BUTTON     DK_BTN1_MSK

static bool app_button_state;

/* Define multiple speed for advertising */
#define SLOOOW_ADV BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME, \
                                   0x1000, 0x1100, NULL)
#define SLOOW_ADV BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME, \
                                   0x500, 0x600, NULL)
#define SLOW_ADV BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME, \
                                 0x200, 0x250, NULL)
#define MID_ADV BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME, \
                                0x100, 0x120, NULL)
#define FAST_ADV BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME, \
                                 0x30, 0x40, NULL)

/* Advertise only on the channel 37 */
#define SLOW_ADV_37 BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME | BT_LE_ADV_OPT_DISABLE_CHAN_38 | BT_LE_ADV_OPT_DISABLE_CHAN_39, \
                                 0x200, 0x250, NULL)


static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_HRS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_DIS_VAL))
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
	} else {
		dk_set_led_on(CON_STATUS_LED);

		struct bt_conn_info info;
		char addr_str[BT_ADDR_LE_STR_LEN];

		bt_conn_get_info(conn, &info);
		bt_addr_le_to_str(info.le.dst, addr_str, sizeof(addr_str));

		printk("Connected with %s\n", addr_str);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_conn_info info;
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_conn_get_info(conn, &info);
	bt_addr_le_to_str(info.le.dst, addr_str, sizeof(addr_str));

	printk("Disconnected with %s (reason 0x%02x)\n", addr_str, reason);
	dk_set_led_off(CON_STATUS_LED);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(void)
{
	int err;

	printk("Bluetooth initialized\n");

	err = bt_le_adv_start(SLOW_ADV_37, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.cancel = auth_cancel,
};

/* Services */

static void bas_notify(void)
{
	uint8_t battery_level = bt_bas_get_battery_level();

	battery_level--;

	if (!battery_level) {
		battery_level = 100U;
	}

	bt_bas_set_battery_level(battery_level);
}

static void hrs_notify(void)
{
	static uint8_t heartrate = 90U;

	/* Heartrate measurements simulation */
	heartrate++;
	if (heartrate == 160U) {
		heartrate = 90U;
	}

	bt_hrs_notify(heartrate);
}


static void app_led_cb(bool led_state)
{
	dk_set_led(USER_LED, led_state);
}

static bool app_button_cb(void)
{
	return app_button_state;
}

static struct bt_lbs_cb lbs_callbacs = {
	.led_cb    = app_led_cb,
	.button_cb = app_button_cb,
};

static void button_changed(uint32_t button_state, uint32_t has_changed)
{
	if (has_changed & USER_BUTTON) {
		uint32_t user_button_state = button_state & USER_BUTTON;

		bt_lbs_send_button_state(user_button_state);
		app_button_state = user_button_state ? true : false;
	}
}

static int init_button(void)
{
	int err;

	err = dk_buttons_init(button_changed);
	if (err) {
		printk("Cannot init buttons (err: %d)\n", err);
	}

	return err;
}


int main(void)
{
	int err;
	int blink_status = 0;
    /* Use a known fixed address to ease development */
    const uint8_t public_addr[BDADDR_SIZE] = {0xaa, 0xaa, 0xef, 0xbe, 0xad, 0xde};

	/* Initialize LEDs and button
	 */
	err = dk_leds_init();
	if (err) {
		printk("LEDs init failed (err %d)\n", err);
		return 0;
	}

	err = init_button();
	if (err) {
		printk("Button init failed (err %d)\n", err);
		return 0;
	}
	printk("LEDs & button initialized\n");

    /* Set advertising address */
    bt_ctlr_set_public_addr((const uint8_t *) &public_addr);

	/* Initialize Bluetooth
	 */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}
	printk("Bluetooth initialized\n");

    /* Initialize LEDs and button callbacks
    */
	err = bt_lbs_init(&lbs_callbacs);
	if (err) {
		printk("Failed to init LBS (err:%d)\n", err);
		return 0;
	}

    /* Initialized Sirocco IDS 
     */
 #if defined(CONFIG_BT_SIROCCO)
	srcc_init();
	printk("Sirocco Bluetooth IDS initialized\n");
#endif

	/* Start advertising */
	bt_ready();
	bt_conn_auth_cb_register(&auth_cb_display);


	/* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
	while (1) {
		k_sleep(K_SECONDS(1));

		/* Blink the LED */
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);

		/* Heartrate measurements simulation */
		hrs_notify();

		/* Battery level simulation */
		bas_notify();
	}
	return 0;
}
