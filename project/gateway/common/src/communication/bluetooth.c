#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>

#include <bluetooth.h>
#include <json.h>

LOG_MODULE_REGISTER(bluetooth_module);

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

struct sensor_packet_t {
    uint8_t data[64];
};

#define MSGQ_MAX_MSGS 10  // adjust if needed
K_MSGQ_DEFINE(sensor_msgq, sizeof(struct sensor_packet_t), MSGQ_MAX_MSGS, 4);

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
    if (!data || length != 64) {
        return BT_GATT_ITER_CONTINUE;
    }

    struct sensor_packet_t pkt;
    memcpy(pkt.data, data, 64);

    int ret = k_msgq_put(&sensor_msgq, &pkt, K_NO_WAIT);
    if (ret != 0) {
        // Optional: handle dropped packet (queue full)
    }

    return BT_GATT_ITER_CONTINUE;
}

extern void process_data_thread(void) {
    struct sensor_packet_t pkt;

    while (1) {
        // Wait forever until a sensor packet arrives in the queue
        k_msgq_get(&sensor_msgq, &pkt, K_FOREVER);

        tracker_payload_t payload = {0};

        // Copy first 32 bytes into hash array (manual copy if avoiding memcpy)
        for (int i = 0; i < 32; i++) {
            payload.hash[i] = pkt.data[i];
        }

        // Unpack timestamp (4 bytes, big endian)
        payload.timestamp = (pkt.data[32] << 24) |
                             (pkt.data[33] << 16) |
                             (pkt.data[34] << 8)  |
                             pkt.data[35];

        // Latitude (4 bytes, int32_t)
        payload.lat = (pkt.data[36] << 24) |
                      (pkt.data[37] << 16) |
                      (pkt.data[38] << 8)  |
                      pkt.data[39];
        payload.ns = (char)pkt.data[40];

        // Longitude (4 bytes, int32_t)
        payload.lon = (pkt.data[41] << 24) |
                      (pkt.data[42] << 16) |
                      (pkt.data[43] << 8)  |
                      pkt.data[44];
        payload.ew = (char)pkt.data[45];

        // Altitude (2 bytes, int16_t)
        payload.alt = (pkt.data[46] << 8) | pkt.data[47];

        // Satellites (1 byte)
        payload.sat = pkt.data[48];

        // Temperature (2 bytes, int16_t)
        payload.temp = (pkt.data[49] << 8) | pkt.data[50];

        // Humidity (2 bytes, int16_t)
        payload.humid = (pkt.data[51] << 8) | pkt.data[52];

        // Pressure (2 bytes, int16_t)
        payload.press = (pkt.data[53] << 8) | pkt.data[54];

        // Gas (2 bytes, int16_t)
        payload.gas = (pkt.data[55] << 8) | pkt.data[56];

        // Accelerometer X, Y, Z (2 bytes each, int16_t)
        payload.x = (pkt.data[57] << 8) | pkt.data[58];
        payload.y = (pkt.data[59] << 8) | pkt.data[60];
        payload.z = (pkt.data[61] << 8) | pkt.data[62];

        // Device ID (1 byte)
        payload.dev_id = pkt.data[63];

        struct json_full_packet json_packet = {0};
        fill_json_packet_from_tracker_payload(&payload, &json_packet);
        //print_json_full_packet(&json_packet);
        encode_and_print_json(&json_packet);
    }
}


// Step 1: Discover characteristic
static uint8_t discover_func(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             struct bt_gatt_discover_params *params) {
    int index = get_conn_index(conn);

    if (!attr) {
        LOG_ERR("[BASE %d] Characteristic not found", index);
        return BT_GATT_ITER_STOP;
    }

    // Save characteristic handle
    const struct bt_gatt_chrc *gatt_chrc = attr->user_data;
    char_handles[index] = gatt_chrc->value_handle;
    LOG_INF("[BASE %d] Char handle: 0x%04x", index, char_handles[index]);

    // Now that characteristic is found, start CCCD descriptor discovery
    cccd_discover_params[index].uuid = &conn_characteristic_uuid.uuid;
    cccd_discover_params[index].start_handle = char_handles[index];
    cccd_discover_params[index].end_handle = 0xffff;
    cccd_discover_params[index].type = BT_GATT_DISCOVER_ATTRIBUTE;
    cccd_discover_params[index].func = cccd_discover_func;

    int err = bt_gatt_discover(conn, &cccd_discover_params[index]);
    if (err) {
        LOG_ERR("[BASE %d] CCCD discover failed (%d)", index, err);
    }

    return BT_GATT_ITER_STOP;  // stop after finding the characteristic
}


// Step 2: Discover CCCD descriptor
static uint8_t cccd_discover_func(struct bt_conn *conn,
                                  const struct bt_gatt_attr *attr,
                                  struct bt_gatt_discover_params *params) {
    
    int index = get_conn_index(conn);

    if (!attr) {
        LOG_ERR("[BASE %d] CCCD not found", index);
        return BT_GATT_ITER_STOP;
    }

    ccc_handles[index] = attr->handle;
    LOG_INF("[BASE %d] CCCD handle: 0x%04x", index, ccc_handles[index]);

    // Setup subscription params
    subs[index].ccc_handle = ccc_handles[index];
    subs[index].value_handle = char_handles[index];
    subs[index].notify = notify_func;
    subs[index].value = BT_GATT_CCC_NOTIFY;

    int err = bt_gatt_subscribe(conn, &subs[index]);
    if (err) {
        LOG_ERR("[BASE %d] Subscribe failed (%d)", index, err);
    } else {
        LOG_INF("[BASE %d] Subscribed to notifications", index);
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
        LOG_ERR("[BASE %d] Discover characteristic failed (%d)", index, err);
    }
}

void exchange_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
    if (err) {
        LOG_ERR("MTU exchange failed (err %u)", err);
    } else {
        uint16_t mtu = bt_gatt_get_mtu(conn);
        LOG_INF("MTU exchanged successfully: %d", mtu);
    }
}

struct bt_gatt_exchange_params mtu_params = {
    .func = exchange_func,
};


static void connected(struct bt_conn *conn, uint8_t err) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
        LOG_ERR("[BASE] Failed to connect to %s (err %u)", addr, err);
        bt_conn_unref(conn);
        start_scan();
        return;
    }

    int index = get_conn_index(conn);
    if (index < 0) {
        LOG_INF("[BASE] No free conn slots");
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        start_scan();
        return;
    }

    LOG_INF("[BASE] Connected [%d]: %s", index, addr);

    // Exchange the MTU
    bt_gatt_exchange_mtu(conn, &mtu_params);

    // start_full_discovery(conn);
    discover_tracker_characteristic(conn, index);
    start_scan();
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    int index = get_conn_index(conn);
    LOG_INF("[BASE] Disconnected [%d] (reason 0x%02x)", index, reason);

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
    LOG_INF("[BASE] Found tracker: %s (RSSI %d)", addr_str, rssi);
    
    bt_le_scan_stop();
    conns[index_available] = NULL;
    int err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
                                BT_LE_CONN_PARAM_DEFAULT, &conns[index_available]);
    if (err) {
        LOG_ERR("[BASE] Failed to connect (%d)", err);
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
        LOG_ERR("[BASE] Scan failed to start (%d)", err);
    } else {
        LOG_INF("[BASE] Scanning for tracker...");
    }
}

void base_thread(void) {
    int err = bt_enable(NULL);
    if (err) {
        LOG_ERR("[BASE] Bluetooth init failed (err %d)", err);
        return;
    }

    LOG_INF("[BASE] Bluetooth initialized");

    bt_conn_cb_register(&conn_callbacks);

    start_scan();
}