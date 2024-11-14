/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/bluetooth/sirocco.h>
#include <zephyr/bluetooth/srcc_time_analysis.h>



int observer_start(void);


#if defined(CONFIG_BT_SIROCCO)
int init_sirocco(void)
{
	/* Initialize Sirocco Zephyr subsystem */
	srcc_init();
	return 0;
}
#endif

int main(void)
{
	int err;

	printk("Starting Observer Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

    /* Initialized latency analysis
     */
#if defined(CONFIG_SRCC_ANALYSIS)
    init_srcc_analysis();
#endif

	/* Initialize Sirocco Bluetooth IDS
     */
#if defined(CONFIG_BT_SIROCCO)
	err = init_sirocco();
	if (err) {
		printk("Sirocco Bluetooth IDS init failed (err %d)\n", err);
		return 0;
	}
	printk("Sirocco Bluetooth IDS initialized\n");
#endif

	(void)observer_start();

	printk("Exiting %s thread.\n", __func__);
	return 0;
}
