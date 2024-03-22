/*
 */
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/addr.h>

#include "detect_btlejuice.h"


#define SRCC_BTLEJUICE_SCAN_INTERVAL    0x0010
#define SRCC_BTLEJUICE_SCAN_WINDOW      0x0010


static bool is_scanning = false;


static void sirocco_btlejuice_cb(const bt_addr_le_t *remote_addr, int8_t rssi, uint8_t adv_type,
		    struct net_buf_simple *buf)
{
    /* Print scanned devices 
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(remote_addr, addr_str, BT_ADDR_LE_STR_LEN);
    // TODO: check returned value by bt_addr_le_to_str
	printk("[BTLEJUICE]: %s, ADV evt type %u, ADV data len %u, RSSI %i\n",
	       addr_str, adv_type, buf->len, rssi);
    */

	char laddr_str[BT_ADDR_LE_STR_LEN];
	char raddr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_t local_addrs[CONFIG_BT_ID_MAX];
	size_t count = 1;
    int res = 0;

	bt_id_get(local_addrs, &count);
    //printk("%d\n", count);

    for (int i=0; i<count; i++) {
        bt_addr_le_to_str(&local_addrs[i], laddr_str, BT_ADDR_LE_STR_LEN);
        bt_addr_le_to_str(remote_addr, raddr_str, BT_ADDR_LE_STR_LEN);

        //if (bt_addr_le_eq(&local_addrs[i], remote_addr)) {
        //if (bt_addr_le_cmp(&local_addrs[i], remote_addr) == 0) {
        //res = bt_addr_le_cmp(&local_addrs[i], remote_addr);
        res = bt_addr_cmp(&remote_addr->a, &local_addrs[i].a);
        if (res == 0) {
            printk(">>> [SIROCCO] BTLEJuice attack detected !!!\n");
        }
        //printk("local: %s ? remote: %s res = %d\n", laddr_str, raddr_str, res);
    }

}

/*
int sirocco_btlejuice_init()
{
    return 0;
}
*/

int sirocco_btlejuice_start()
{
    int err;
    struct bt_le_scan_param scan_param = {
        .type       = BT_LE_SCAN_TYPE_PASSIVE,
        .options    = BT_LE_SCAN_OPT_NONE,
        .interval   = SRCC_BTLEJUICE_SCAN_INTERVAL,
        .window     = SRCC_BTLEJUICE_SCAN_WINDOW,
    };

    /* Start scanning */
	err = bt_le_scan_start(&scan_param, sirocco_btlejuice_cb);
	if (err) {
		printk("Starting scanning failed (err %d)\n", err);
		return 1;
	}

    is_scanning = true;
    printk("Sirocco BTLEJuice module started\n");

    return 0;
}

int sirocco_btlejuice_stop()
{
    int err;

	err = bt_le_scan_stop();
	if (err) {
		printk("Stopping scanning failed (err %d)\n", err);
		return 1;
	}

    is_scanning = false;
    printk("Sirocco BTLEJuice module stopped\n");

    return 0;
}
