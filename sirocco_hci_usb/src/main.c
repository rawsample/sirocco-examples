/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

#include <zephyr/bluetooth/sirocco.h>


#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
USBD_CONFIGURATION_DEFINE(config_1,
			  USB_SCD_SELF_POWERED,
			  200);

USBD_DESC_LANG_DEFINE(sample_lang);
USBD_DESC_MANUFACTURER_DEFINE(sample_mfr, "ZEPHYR");
USBD_DESC_PRODUCT_DEFINE(sample_product, "Zephyr USBD BT HCI");
USBD_DESC_SERIAL_NUMBER_DEFINE(sample_sn, "0123456789ABCDEF");


USBD_DEVICE_DEFINE(sample_usbd,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   0x2fe3, 0x000b);

static int enable_usb_device_next(void)
{
	int err;

	err = usbd_add_descriptor(&sample_usbd, &sample_lang);
	if (err) {
		return err;
	}

	err = usbd_add_descriptor(&sample_usbd, &sample_mfr);
	if (err) {
		return err;
	}

	err = usbd_add_descriptor(&sample_usbd, &sample_product);
	if (err) {
		return err;
	}

	err = usbd_add_descriptor(&sample_usbd, &sample_sn);
	if (err) {
		return err;
	}

	err = usbd_add_configuration(&sample_usbd, &config_1);
	if (err) {
		return err;
	}

	err = usbd_register_class(&sample_usbd, "bt_hci_0", 1);
	if (err) {
		return err;
	}

	err = usbd_init(&sample_usbd);
	if (err) {
		return err;
	}

	err = usbd_enable(&sample_usbd);
	if (err) {
		return err;
	}

	return 0;
}
#endif /* CONFIG_USB_DEVICE_STACK_NEXT */

#if defined(CONFIG_BT_SIROCCO)
static int init_sirocco(void)
{
	/* Initialize Sirocco Zephyr subsystem */
	srcc_init();
 
    /* Register callbacks
    TODO
	srcc_cb_register(&cb);
    */

	return 0;
}
#endif

int main(void)
{
	int ret;

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
	ret = enable_usb_device_next();
#else
	ret = usb_enable(NULL);
#endif

	if (ret != 0) {
		printk("Failed to enable USB");
		return 0;
	}

	/* Initialize Sirocco Bluetooth IDS
     */
#if defined(CONFIG_BT_SIROCCO)
	ret = init_sirocco();
	if (ret) {
		printk("Sirocco Bluetooth IDS init failed (err %d)\n", ret);
		return 0;
	}
	printk("Sirocco Bluetooth IDS initialized\n");
#endif

	printk("Bluetooth over USB sample\n");
	return 0;
}
