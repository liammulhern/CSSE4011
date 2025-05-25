#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>

#include <tinycrypt/sha256.h>
#include <tinycrypt/constants.h>
#include <string.h>

#include <bluetooth.h>

static struct bt_conn *current_conn;
static bool notify_enabled = false;  // NEW

#define DEV_ID 1

// Global buffer holding packed sensor data (14 bytes)
#define PACKED_DATA_LEN 64
uint8_t packed_data[PACKED_DATA_LEN];

// THIS WILL BE REMOVED AND INCLUDED IN ACTUAL FILE SET UP
// -----------------------------------------------------------------
struct sensor_blk {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    float lat;
    char ns;
    float lon;
    char ew;
    float alt; 
    int sat;
    double temp;
    double hum;
    double press;
    double gas;
    double x_accel; 
    double y_accel;
    double z_accel;
};
// -----------------------------------------------------------------

// 'digest' must be a buffer at least 32 bytes long (TC_SHA256_DIGEST_SIZE)
void hash_sensor_blk(const struct sensor_blk *data, uint8_t *digest)
{
    struct tc_sha256_state_struct sha256_ctx;

    if (tc_sha256_init(&sha256_ctx) != TC_CRYPTO_SUCCESS) {
        printk("SHA256 init failed\n");
        return;
    }

    if (tc_sha256_update(&sha256_ctx, (const uint8_t *)data, sizeof(struct sensor_blk)) != TC_CRYPTO_SUCCESS) {
        printk("SHA256 update failed\n");
        return;
    }

    if (tc_sha256_final(digest, &sha256_ctx) != TC_CRYPTO_SUCCESS) {
        printk("SHA256 final failed\n");
        return;
    }
}

void print_sha256_digest(const uint8_t *digest)
{
    printk("SHA-256 digest: ");
    for (int i = 0; i < TC_SHA256_DIGEST_SIZE; i++) {
        printk("%02x", digest[i]);
    }
    printk("\n");
}

// Characteristic read callback returns current packed sensor data
static ssize_t pack_data(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                         void *buf, uint16_t len, uint16_t offset) {
    printk("[TRACKER] packing data\n");
    return bt_gatt_attr_read(conn, attr, buf, len, offset, packed_data, PACKED_DATA_LEN);
}

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {

    if (value == BT_GATT_CCC_NOTIFY) {
        notify_enabled = true;
        printk("Notifications enabled\n");
    } else {
        printk("Notifications disabled\n");
        notify_enabled = false;
    }
}

BT_GATT_SERVICE_DEFINE(custom_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_128(
        0x12, 0x34, 0x56, 0x78,
        0x12, 0x34,
        0x56, 0x78,
        0x12, 0x34,
        0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0)),
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128(
        0x99, 0x88, 0x77, 0x66,
        0x55, 0x44,
        0x33, 0x22,
        0x11, 0x00,
        0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa),
        BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
        BT_GATT_PERM_READ,
        pack_data,
        NULL, NULL),
    BT_GATT_CCC(ccc_cfg_changed,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

// Connection callbacks
static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("[TRACKER] Failed to connect (err %u)\n", err);
        return;
    }

    current_conn = bt_conn_ref(conn);
    printk("[TRACKER] Connected\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("[TRACKER] Disconnected (reason 0x%02x)\n", reason);
    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }
}

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

// Custom 128-bit service UUID for advertising (little endian)
static const uint8_t custom_service_uuid[16] = {
    0x12, 0x34, 0x56, 0x78,
    0x12, 0x34, 0x56, 0x78,
    0x12, 0x34, 0x56, 0x78,
    0x9a, 0xbc, 0xde, 0xf0,
};

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_UUID128_ALL, custom_service_uuid, sizeof(custom_service_uuid)),
};

static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, strlen(CONFIG_BT_DEVICE_NAME)),
};

struct bt_le_adv_param adv_params = {
    .id = BT_ID_DEFAULT,
    .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
    .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
    .options = BT_LE_ADV_OPT_CONNECTABLE,
};

static void print_packed_data() {

    // Unpack sensor fields starting from index 32
    uint8_t hour   = packed_data[32];
    uint8_t minute = packed_data[33];
    uint8_t second = packed_data[34];

    int32_t lat = (packed_data[35] << 24) | (packed_data[36] << 16) |
                  (packed_data[37] << 8)  | packed_data[38];
    char ns = (char)packed_data[39];

    int32_t lon = (packed_data[40] << 24) | (packed_data[41] << 16) |
                  (packed_data[42] << 8)  | packed_data[43];
    char ew = (char)packed_data[44];

    int16_t alt = (packed_data[45] << 8) | packed_data[46];
    uint8_t sat = packed_data[47];

    int16_t temp = (packed_data[48] << 8) | packed_data[49];
    int16_t hum  = (packed_data[50] << 8) | packed_data[51];

    int16_t press = (packed_data[52] << 8) | packed_data[53];
    int16_t gas   = (packed_data[54] << 8) | packed_data[55];

    int16_t x = (packed_data[56] << 8) | packed_data[57];
    int16_t y = (packed_data[58] << 8) | packed_data[59];
    int16_t z = (packed_data[60] << 8) | packed_data[61];

    printk("[TRACKER] Parsed data:\n");

    // Print first 8 bytes of hash as example (SHA-256 is 32 bytes)
    printk("[TRACKER] Hash: ");
    for (int i = 0; i < 32; i++) {
        printk("%02X ", packed_data[i]);
    }
    printk("\n");

    printk("  Time: %02d:%02d:%02d\n", hour, minute, second);
    printk("  Latitude: %.7f %c\n", lat / 1e7, ns);
    printk("  Longitude: %.7f %c\n", lon / 1e7, ew);
    printk("  Altitude: %.1f m\n", alt / 10.0);
    printk("  Satellites: %d\n", sat);
    printk("  Temperature: %.2f °C\n", temp / 100.0);
    printk("  Humidity: %.2f %%\n", hum / 100.0);
    printk("  Pressure: %.1f hPa\n", press / 10.0);
    printk("  Gas: %.1f units\n", gas / 100.0);
    printk("  Accel X: %.3f m/s²\n", x / 1000.0);
    printk("  Accel Y: %.3f m/s²\n", y / 1000.0);
    printk("  Accel Z: %.3f m/s²\n", z / 1000.0);
}

void pack_sensor_data(const struct sensor_blk *sensor) {
    // Fill bytes 0..31 with SHA-256 digest of sensor block
    hash_sensor_blk(sensor, packed_data);  // hash_sensor_blk(sensor, uint8_t *digest)

    // Bytes 32..61: pack sensor data fields in order, fixing offsets:

    packed_data[32] = sensor->hour;
    packed_data[33] = sensor->minute;
    packed_data[34] = sensor->second;  // fixed index (was 4)

    // Latitude (float → int32: deg * 1e7)
    int32_t lat_fixed = (int32_t)(sensor->lat * 1e7f);
    packed_data[35] = (lat_fixed >> 24) & 0xFF;
    packed_data[36] = (lat_fixed >> 16) & 0xFF;
    packed_data[37] = (lat_fixed >> 8) & 0xFF;
    packed_data[38] = lat_fixed & 0xFF;
    packed_data[39] = sensor->ns;

    // Longitude (float → int32: deg * 1e7)
    int32_t lon_fixed = (int32_t)(sensor->lon * 1e7f);
    packed_data[40] = (lon_fixed >> 24) & 0xFF;
    packed_data[41] = (lon_fixed >> 16) & 0xFF;
    packed_data[42] = (lon_fixed >> 8) & 0xFF;
    packed_data[43] = lon_fixed & 0xFF;
    packed_data[44] = sensor->ew;

    // Altitude (float → int16: m * 10)
    int16_t alt_fixed = (int16_t)(sensor->alt * 10.0f);
    packed_data[45] = (alt_fixed >> 8) & 0xFF;
    packed_data[46] = alt_fixed & 0xFF;

    // Satellites
    packed_data[47] = (uint8_t)(sensor->sat);

    // Temperature (°C × 100)
    int16_t temp_fixed = (int16_t)(sensor->temp * 100.0);
    packed_data[48] = (temp_fixed >> 8) & 0xFF;
    packed_data[49] = temp_fixed & 0xFF;

    // Humidity (% × 100)
    int16_t hum_fixed = (int16_t)(sensor->hum * 100.0);
    packed_data[50] = (hum_fixed >> 8) & 0xFF;
    packed_data[51] = hum_fixed & 0xFF;

    // Pressure (hPa × 10)
    int16_t press_fixed = (int16_t)(sensor->press * 10.0);
    packed_data[52] = (press_fixed >> 8) & 0xFF;
    packed_data[53] = press_fixed & 0xFF;

    // Gas (arbitrary units × 100)
    int16_t gas_fixed = (int16_t)(sensor->gas * 100);
    packed_data[54] = (gas_fixed >> 8) & 0xFF;
    packed_data[55] = gas_fixed & 0xFF;

    // Accelerometer X/Y/Z (m/s² × 1000)
    int16_t x_fixed = (int16_t)(sensor->x_accel * 1000.0);
    int16_t y_fixed = (int16_t)(sensor->y_accel * 1000.0);
    int16_t z_fixed = (int16_t)(sensor->z_accel * 1000.0);

    packed_data[56] = (x_fixed >> 8) & 0xFF;
    packed_data[57] = x_fixed & 0xFF;

    packed_data[58] = (y_fixed >> 8) & 0xFF;
    packed_data[59] = y_fixed & 0xFF;

    packed_data[60] = (z_fixed >> 8) & 0xFF;
    packed_data[61] = z_fixed & 0xFF;

    // Zero padding bytes 62, 63
    packed_data[62] = DEV_ID & 0xFF;
    packed_data[63] = 0;

    // Send notification if enabled and connected
    if (notify_enabled && current_conn) {
        int err = bt_gatt_notify(current_conn, &custom_svc.attrs[1], packed_data, PACKED_DATA_LEN);
        if (err) {
            printk("[TRACKER] Failed to send notification (err %d)\n", err);
        } else {
            printk("[TRACKER] Notification sent\n");
        }
    }
}

int init_bluetooth(void) {
    int err;
    printk("[TRACKER] Initializing Bluetooth...\n");
    err = bt_enable(NULL);
    if (err) {
        printk("[TRACKER] Bluetooth init failed (err %d)\n", err);
        return err;
    }

    bt_conn_cb_register(&conn_callbacks);

    return 0;
}

int start_advertising(void) {
    int err;
    printk("[TRACKER] Starting advertising...\n");
    err = bt_le_adv_start(&adv_params, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        printk("[TRACKER] Advertising failed to start (err %d)\n", err);
        return err;
    }

    printk("[TRACKER] Advertising started\n");
    return 0;
}

int stop_advertising(void) {
    int err;
    printk("[TRACKER] Stopping advertising...\n");

    err = bt_le_adv_stop();
    if (err) {
        printk("[TRACKER] Failed to stop advertising (err %d)\n", err);
        return err;
    }

    printk("[TRACKER] Advertising stopped\n");
    return 0;
}


void tracker_thread(void) {
    int err;

    err = init_bluetooth();
    if (err) {
        printk("[TRACKER] Bluetooth init failed\n");
        return;
    }

    // Example sensor data
    struct sensor_blk sensor = {
        .hour = 12,
        .minute = 34,
        .second = 56,
        .lat = 37.7699999f,
        .ns = 'N',
        .lon = -122.419998f,
        .ew = 'W',
        .alt = 12.3f,
        .sat = 7,
        .temp = 23.50,
        .hum = 45.67,
        .press = 1013.2,
        .gas = 52.3,
        .x_accel = -0.900,
        .y_accel = 0.200,
        .z_accel = 0.985
    };

    uint8_t digest[TC_SHA256_DIGEST_SIZE];
    hash_sensor_blk(&sensor, digest);

    print_sha256_digest(digest);

    while (1) {
        err = start_advertising();
        if (err) {
            printk("[TRACKER] Advertising failed to start\n");
            return;
        }
        k_msleep(3000);  // Wait before restarting next advertisement cycle
        // Advertise sensor data 5 times, once per second
        for (int i = 0; i < 5; i++) {
            pack_sensor_data(&sensor);
            print_packed_data();

            // Simulate sensor updates
            sensor.second = (sensor.second + 1) % 60;
            sensor.temp += 0.1;
            sensor.hum += 0.1;
            sensor.press += 0.1;

            // k_msleep(100); 
        }
        k_msleep(1500); 
        err = stop_advertising();
        if (err) {
            printk("[TRACKER] Failed to stop advertising\n");
        }

        if (current_conn) {
            err = bt_conn_disconnect(current_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
            if (err) {
                printk("[TRACKER] Disconnection failed (err %d)\n", err);
            } else {
                printk("[TRACKER] Disconnection initiated\n");
            }
        }

        printk("[TRACKER] Sleeping before next cycle...\n");
        k_sleep(K_SECONDS(10));  // Wait before restarting next advertisement cycle
    }
}

void tracker_thread(void) {
    int err;

    err = init_bluetooth();
    if (err) {
        printk("[TRACKER] Bluetooth init failed\n");
        return;
    }

    // Example sensor data
    struct sensor_blk sensor = {
        .hour = 12,
        .minute = 34,
        .second = 56,
        .lat = 37.7699999f,
        .ns = 'N',
        .lon = -122.419998f,
        .ew = 'W',
        .alt = 12.3f,
        .sat = 7,
        .temp = 23.50,
        .hum = 45.67,
        .press = 1013.2,
        .gas = 52.3,
        .x_accel = -0.900,
        .y_accel = 0.200,
        .z_accel = 0.985
    };

    uint8_t digest[TC_SHA256_DIGEST_SIZE];
    hash_sensor_blk(&sensor, digest);

    print_sha256_digest(digest);

    while (1) {
        err = start_advertising();
        if (err) {
            printk("[TRACKER] Advertising failed to start\n");
            return;
        }
        k_msleep(3000);  // Wait before restarting next advertisement cycle
        // Advertise sensor data 5 times, once per second
        for (int i = 0; i < 5; i++) {
            pack_sensor_data(&sensor);
            print_packed_data();

            // Simulate sensor updates
            sensor.second = (sensor.second + 1) % 60;
            sensor.temp += 0.1;
            sensor.hum += 0.1;
            sensor.press += 0.1;

            // k_msleep(100); 
        }
        k_msleep(1500); 
        err = stop_advertising();
        if (err) {
            printk("[TRACKER] Failed to stop advertising\n");
        }

        if (current_conn) {
            err = bt_conn_disconnect(current_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
            if (err) {
                printk("[TRACKER] Disconnection failed (err %d)\n", err);
            } else {
                printk("[TRACKER] Disconnection initiated\n");
            }
        }

        printk("[TRACKER] Sleeping before next cycle...\n");
        k_sleep(K_SECONDS(10));  // Wait before restarting next advertisement cycle
    }
}


#define TRACKER_CONTROL_STACK_SIZE 2048
#define TRACKER_CONTROL_PRIORITY 5



// void tracker_thread(void) {

//     int err;

//     err = init_bluetooth();
//     if (err) {
//         return;
//     }
    
//     err = start_advertising();
//         if (err) {
//         return;
//     }

//     // Example sensor data
//     struct sensor_blk sensor = {
//         .hour = 12,
//         .minute = 34,
//         .second = 56,
//         .lat = 37.7699999f,
//         .ns = 'N',
//         .lon = -122.419998f,
//         .ew = 'W',
//         .alt = 12.3f,
//         .sat = 7,
//         .temp = 23.50,
//         .hum = 45.67,
//         .press = 1013.2,
//         .gas = 52.3,
//         .x_accel = -0.900,
//         .y_accel = 0.200,
//         .z_accel = 0.985
//     };

//     // Pack the initial sensor data
//     pack_sensor_data(&sensor);

//     // Print packed bytes and parsed content
//     print_packed_data();

//     while (1) {
//         k_sleep(K_SECONDS(1));

//         // Simulate sensor updates
//         sensor.second = (sensor.second + 1) % 60;
//         sensor.temp += 0.1;
//         sensor.hum += 0.1;
//         sensor.press += 0.1;

//         // Re-pack updated data
//         pack_sensor_data(&sensor);
//     }
// }