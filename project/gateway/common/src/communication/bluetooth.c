#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>

#include <bluetooth.h>

// UUIDs for tracker service and characteristic
static struct bt_uuid_128 tracker_service_uuid = BT_UUID_INIT_128(
    0x12, 0x34, 0x56, 0x78,
    0x12, 0x34,
    0x56, 0x78,
    0x12, 0x34,
    0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0);

static struct bt_uuid_128 tracker_char_uuid = BT_UUID_INIT_128(
    0x99, 0x88, 0x77, 0x66,
    0x55, 0x44,
    0x33, 0x22,
    0x11, 0x00,
    0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa);

static struct bt_uuid_16 conn_characteristic_uuid = BT_UUID_INIT_16(0x2902);

#define MAX_CONN 4  // Maximum peripherals to connect to

static struct bt_conn *conns[MAX_CONN];


static struct bt_gatt_subscribe_params subs[MAX_CONN];
static uint16_t char_handles[MAX_CONN];
static uint16_t ccc_handles[MAX_CONN];

// Discovery parameters per connection (could be static or allocated per connection)
struct bt_gatt_discover_params discover_params[MAX_CONN];
struct bt_gatt_discover_params cccd_discover_params[MAX_CONN];

/** FUNCTION PROTOTYPES*/
static uint8_t discover_func(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             struct bt_gatt_discover_params *params);

static uint8_t cccd_discover_func(struct bt_conn *conn,
                                  const struct bt_gatt_attr *attr,
                                  struct bt_gatt_discover_params *params);

// Forward declaration
static void start_scan(void);

static int get_conn_index(struct bt_conn *conn) {
    for (int i = 0; i < MAX_CONN; i++) {
        if (conns[i] == conn) {
            return i;
        }
    }
    return -1;
}

// Advertisement UUID filter
static bool is_uuid_in_ad(struct net_buf_simple *ad, const struct bt_uuid *uuid) {
    size_t offset = 0;
    while (offset < ad->len) {
        uint8_t len = ad->data[offset];
        if (len == 0 || (offset + len) >= ad->len) break;
        uint8_t type = ad->data[offset + 1];
        const uint8_t *data = &ad->data[offset + 2];
        uint8_t data_len = len - 1;
        if (type == BT_DATA_UUID128_ALL || type == BT_DATA_UUID128_SOME) {
            for (size_t i = 0; i + 16 <= data_len; i += 16) {
                struct bt_uuid_128 adv_uuid;
                memcpy(adv_uuid.val, &data[i], 16);
                adv_uuid.uuid.type = BT_UUID_TYPE_128;
                if (!bt_uuid_cmp(uuid, &adv_uuid.uuid)) {
                    return true;
                }
            }
        }
        offset += len + 1;
    }
    return false;
}

static uint8_t notify_func(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
                           const void *data, uint16_t length) {
    int index = get_conn_index(conn);

    if (!data || length != 64) {  // Now expect 64 bytes total
        printk("[TRACKER %d] Invalid or no notification data (len = %d)\n", index, length);
        return BT_GATT_ITER_CONTINUE;
    }

    const uint8_t *d = data;

    // Unpack sensor data starting at offset 32
    uint8_t hour   = d[32];
    uint8_t minute = d[33];
    uint8_t second = d[34];

    int32_t lat = (d[35] << 24) | (d[36] << 16) | (d[37] << 8) | d[38];
    char ns = (char)d[39];

    int32_t lon = (d[40] << 24) | (d[41] << 16) | (d[42] << 8) | d[43];
    char ew = (char)d[44];

    int16_t alt = (d[45] << 8) | d[46];
    uint8_t sat = d[47];

    int16_t temp = (d[48] << 8) | d[49];
    int16_t hum  = (d[50] << 8) | d[51];

    int16_t press = (d[52] << 8) | d[53];
    int16_t gas   = (d[54] << 8) | d[55];

    int16_t x = (d[56] << 8) | d[57];
    int16_t y = (d[58] << 8) | d[59];
    int16_t z = (d[60] << 8) | d[61];

    printk("[TRACKER %d] Time: %02d:%02d:%02d\n", index, hour, minute, second);
    printk("[TRACKER %d] Lat: %.7f %c | Lon: %.7f %c | Alt: %.1f m | Sats: %d\n",
       index, lat / 1e7, ns, lon / 1e7, ew, alt / 10.0, sat);

    printk("[TRACKER %d] Temp: %.2f °C | Hum: %.2f %% | Press: %.1f hPa | Gas: %.2f\n",
       index, temp / 100.0, hum / 100.0, press / 10.0, gas / 100.0);

    printk("[TRACKER %d] Accel: X=%.3f Y=%.3f Z=%.3f m/s²\n",
       index, x / 1000.0, y / 1000.0, z / 1000.0);


    // // Print hash (SHA-256 is 32 bytes)
    // printk("[TRACKER] Hash: ");
    // for (int i = 0; i < 32; i++) {
    //     printk("%02X ", d[i]);
    // }
    // printk("\n");


    return BT_GATT_ITER_CONTINUE;
}


// Step 1: Discover characteristic
static uint8_t discover_func(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             struct bt_gatt_discover_params *params) {
    int index = get_conn_index(conn);

    if (!attr) {
        printk("[BASE %d] Characteristic not found\n", index);
        return BT_GATT_ITER_STOP;
    }

    printk("called1\n");

    // Save characteristic handle
    const struct bt_gatt_chrc *gatt_chrc = attr->user_data;
    char_handles[index] = gatt_chrc->value_handle;
    printk("[BASE %d] Char handle: 0x%04x\n", index, char_handles[index]);

    // Now that characteristic is found, start CCCD descriptor discovery
    cccd_discover_params[index].uuid = &conn_characteristic_uuid.uuid;
    cccd_discover_params[index].start_handle = char_handles[index];
    cccd_discover_params[index].end_handle = 0xffff;
    cccd_discover_params[index].type = BT_GATT_DISCOVER_ATTRIBUTE;
    cccd_discover_params[index].func = cccd_discover_func;

    int err = bt_gatt_discover(conn, &cccd_discover_params[index]);
    if (err) {
        printk("[BASE %d] CCCD discover failed (%d)\n", index, err);
    }

    return BT_GATT_ITER_STOP;  // stop after finding the characteristic
}


// Step 2: Discover CCCD descriptor
static uint8_t cccd_discover_func(struct bt_conn *conn,
                                  const struct bt_gatt_attr *attr,
                                  struct bt_gatt_discover_params *params) {
    printk("called\n");
    
    int index = get_conn_index(conn);

    if (!attr) {
        printk("[BASE %d] CCCD not found\n", index);
        return BT_GATT_ITER_STOP;
    }

    ccc_handles[index] = attr->handle;
    printk("[BASE %d] CCCD handle: 0x%04x\n", index, ccc_handles[index]);

    // Setup subscription params
    subs[index].ccc_handle = ccc_handles[index];
    subs[index].value_handle = char_handles[index];
    subs[index].notify = notify_func;
    subs[index].value = BT_GATT_CCC_NOTIFY;

    int err = bt_gatt_subscribe(conn, &subs[index]);
    if (err) {
        printk("[BASE %d] Subscribe failed (%d)\n", index, err);
    } else {
        printk("[BASE %d] Subscribed to notifications\n", index);
    }

    return BT_GATT_ITER_STOP;
}

// Call this function to start discovery chain
static void discover_tracker_characteristic(struct bt_conn *conn, int index)
{
    discover_params[index].uuid = &tracker_char_uuid.uuid;
    discover_params[index].func = discover_func;
    discover_params[index].start_handle = 0x0001;
    discover_params[index].end_handle = 0xffff;
    discover_params[index].type = BT_GATT_DISCOVER_CHARACTERISTIC;

    int err = bt_gatt_discover(conn, &discover_params[index]);
    if (err) {
        printk("[BASE %d] Discover characteristic failed (%d)\n", index, err);
    }
}

void exchange_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
    if (err) {
        printk("MTU exchange failed (err %u)\n", err);
    } else {
        uint16_t mtu = bt_gatt_get_mtu(conn);
        printk("MTU exchanged successfully: %d\n", mtu);
    }
}

struct bt_gatt_exchange_params mtu_params = {
    .func = exchange_func,
};

static void connected(struct bt_conn *conn, uint8_t err) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
        printk("[BASE] Failed to connect to %s (err %u)\n", addr, err);
        bt_conn_unref(conn);
        start_scan();
        return;
    }

    int index = get_conn_index(conn);
    if (index < 0) {
        printk("[BASE] No free conn slots\n");
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        start_scan();
        return;
    }

    printk("[BASE] Connected [%d]: %s\n", index, addr);

    // Exchange the MTU
    bt_gatt_exchange_mtu(conn, &mtu_params);

    // start_full_discovery(conn);
    discover_tracker_characteristic(conn, index);
    start_scan();
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    int index = get_conn_index(conn);
    printk("[BASE] Disconnected [%d] (reason 0x%02x)\n", index, reason);

    if (index >= 0) {
        bt_conn_unref(conns[index]);
        conns[index] = NULL;
        char_handles[index] = 0;
    }
}

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                         struct net_buf_simple *ad) {
    if (type != BT_GAP_ADV_TYPE_ADV_IND &&
        type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
        return;
    }

    if (!is_uuid_in_ad(ad, &tracker_service_uuid.uuid)) {
        return;
    }

    bool slot_available = false;
    int index_available;
    for (int i = 0; i < MAX_CONN; i++) {
        if (!conns[i]) {
            slot_available = true;
            index_available = i;
            break;
        }
    }

    if (!slot_available) {
        // All slots full, do nothing
        return;
    }

    char addr_str[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
    printk("[BASE] Found tracker: %s (RSSI %d)\n", addr_str, rssi);
    
    bt_le_scan_stop();
    conns[index_available] = NULL;
    int err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
                                BT_LE_CONN_PARAM_DEFAULT, &conns[index_available]);
    if (err) {
        printk("[BASE] Failed to connect (%d)\n", err);
        start_scan();
    }
}

static void start_scan(void) {
    struct bt_le_scan_param scan_params = {
        .type = BT_HCI_LE_SCAN_PASSIVE,
        .options = BT_LE_SCAN_OPT_NONE,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window = BT_GAP_SCAN_FAST_WINDOW,
    };

    int err = bt_le_scan_start(&scan_params, device_found);
    if (err) {
        printk("[BASE] Scan failed to start (%d)\n", err);
    } else {
        printk("[BASE] Scanning for tracker...\n");
    }
}

void base_thread(void) {
    int err = bt_enable(NULL);
    if (err) {
        printk("[BASE] Bluetooth init failed (err %d)\n", err);
        return;
    }

    printk("[BASE] Bluetooth initialized\n");

    bt_conn_cb_register(&conn_callbacks);

    start_scan();
}



// static struct bt_gatt_discover_params full_disc_params;

// static uint8_t full_discover_func(struct bt_conn *conn,
//                                    const struct bt_gatt_attr *attr,
//                                    struct bt_gatt_discover_params *params) {
//     if (!attr) {
//         printk("[BASE] Full discovery complete\n");
//         return BT_GATT_ITER_STOP;
//     }

//     printk("[BASE] Found attribute: handle 0x%04x, UUID ", attr->handle);

//     if (attr->uuid->type == BT_UUID_TYPE_16) {
//         printk("0x%04x", BT_UUID_16(attr->uuid)->val);
//     } else if (attr->uuid->type == BT_UUID_TYPE_128) {
//         char uuid_str[BT_UUID_STR_LEN];
//         bt_uuid_to_str(attr->uuid, uuid_str, sizeof(uuid_str));
//         printk("%s", uuid_str);
//     } else {
//         printk("Unknown UUID type");
//     }

//     printk("\n");

//     return BT_GATT_ITER_CONTINUE;
// }

// static void start_full_discovery(struct bt_conn *conn) {
//     memset(&full_disc_params, 0, sizeof(full_disc_params));
//     full_disc_params.uuid = NULL; // Discover all
//     full_disc_params.start_handle = 0x0001;
//     full_disc_params.end_handle = 0xffff;
//     full_disc_params.type = BT_GATT_DISCOVER_ATTRIBUTE;
//     full_disc_params.func = full_discover_func;

//     int err = bt_gatt_discover(conn, &full_disc_params);
//     if (err) {
//         printk("[BASE] Full discovery failed: %d\n", err);
//     } else {
//         printk("[BASE] Starting full attribute discovery\n");
//     }
// }