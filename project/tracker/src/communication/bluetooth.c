#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/timeutil.h>

//Cryptographic libraries for SHA-256 hash uti
#include <tinycrypt/sha256.h>
#include <tinycrypt/constants.h>

#include <string.h>

#include <bluetooth.h>
#include <json.h>

#define DEV_ID 2
#define PACKED_DATA_LEN 68

/* GLOBAL VARIABLES*/
uint8_t packed_data[PACKED_DATA_LEN];
static struct bt_conn *current_conn;
static bool notify_enabled = false;  // NEW
uint8_t ble_tick = 0;
uint8_t dev_id = DEV_ID;

// Custom 128-bit service UUID for advertising (little endian)
static const uint8_t custom_service_uuid[16] = {
    0x12, 0x34, 0x56, 0x78,
    0x12, 0x34, 0x56, 0x78,
    0x12, 0x34, 0x56, 0x78,
    0x9a, 0xbc, 0xde, 0xf0,
};

// Advertising data: Flags for general discovery and custome 128-bit service UUID to display (for base to see then establish gatt connection)
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_UUID128_ALL, custom_service_uuid, sizeof(custom_service_uuid)),
};

// Scan response data: Complete device name from configuration (only using for testing on nrf app)
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, strlen(CONFIG_BT_DEVICE_NAME)),
};

/* FUNCTION PROTOTYPES*/
extern uint8_t get_ble_tick(void);
extern void set_ble_tick(uint8_t value);
void hash_sensor_blk(const struct sensor_blk *data, uint8_t *hash_buffer);
void print_sha256_digest(const uint8_t *hash_buffer);
static ssize_t pack_data(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                         void *buf, uint16_t len, uint16_t offset);
static ssize_t write_handler(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             const void *buf, uint16_t len,
                             uint16_t offset, uint8_t flags);
static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void print_packed_data();
extern void pack_sensor_data(const struct sensor_blk *sensor);
extern int init_bluetooth(void);
extern int start_advertising(void);
extern int stop_advertising(void);
extern int stop_advertising_and_disconnect();
void tracker_thread(void);
static void fill_json_packet_from_tracker_payload(const struct sensor_blk *data, char* json_output);

// Advertising parameters
struct bt_le_adv_param adv_params = {
    .id = BT_ID_DEFAULT,
    .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
    .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
    .options = BT_LE_ADV_OPT_CONNECTABLE, // Connectable mode (this is depreciatetd though only thing i found to work)
};

//Bluetooth connection callbacks structure:
static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

/*
GATT (Generic Attribute Profile) is a protocol used in Bluetooth Low Energy (BLE) 
defining how data is structured, discovered, and exchanged between devices. 
Data is organised into services and characteristics.

- Services are collections of data and associated behaviors (like a sensor service).
- Characteristics are individual data points or features within a service (like a temperature value).

Each attribute (service, characteristic, descriptor, etc.) in GATT has a unique handle,
A 16-bit identifier used by the BLE stack to read, write, or discover that attribute. 
Handles allow clients to specifically refer and operate on specific data points.
*/
BT_GATT_SERVICE_DEFINE(custom_svc, //custom service
    BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_128( //primary service with 128-bit custom UUID
        0x12, 0x34, 0x56, 0x78,
        0x12, 0x34,
        0x56, 0x78,
        0x12, 0x34,
        0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0)),
    BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_128( //characteristic defined within the service
        0x99, 0x88, 0x77, 0x66,
        0x55, 0x44,
        0x33, 0x22,
        0x11, 0x00,
        0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa),
        BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY, // read, write and notifying characteristic permissions
        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, // read and write attribute permissions
        pack_data, //read callback
        write_handler, //write callback
        NULL),
    // Client Characteristic Configuration Descriptor (CCCD)
    // Allows the client to enable or disable notifications
    BT_GATT_CCC(ccc_cfg_changed,
                BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

//Helper functions

/**
 * @brief Get the current value of the BLE tick counter.
 *
 * @return The current value of the ble_tick variable.
 */
extern uint8_t get_ble_tick(void) {
    return ble_tick;
}

/**
 * @brief Set the BLE tick counter to a specified value.
 *
 * @param value The new value to assign to the ble_tick variable.
 */
extern void set_ble_tick(uint8_t value) {
    ble_tick = value;
}

/**
 * @brief Compute the SHA-256 hash of a sensor_blk data structure.
 *
 * This function initializes a SHA-256 hashing context, updates it with the
 * bytes of the provided sensor_blk structure, and then finalizes the hash
 * computation to produce a 32-byte hash. 
 * Hash computed on tracker node to secure data for later use in blockchain
 *
 * @param data   Pointer to the sensor_blk structure to be hashed.
 * @param hash_buffer Pointer to a buffer (at least 32 bytes) where the resulting
 *               SHA-256 hash will be stored.
 */
void hash_sensor_blk(const struct sensor_blk *data, uint8_t *hash_buffer) {
    struct tc_sha256_state_struct sha256_ctx;

    // Initialize SHA-256 context
    if (tc_sha256_init(&sha256_ctx) != TC_CRYPTO_SUCCESS) {
        printk("SHA256 init failed\n");
        return;
    }

    char json_output[JSON_BUFFER_SIZE];
    fill_json_packet_from_tracker_payload(data, json_output);

    // Update hash context with sensor_blk data bytes
    if (tc_sha256_update(&sha256_ctx, (const uint8_t *)json_output, sizeof(json_output)) != TC_CRYPTO_SUCCESS) {
        printk("SHA256 update failed\n");
        return;
    }

    // Finalize the hash computation and output the result into digest buffer
    if (tc_sha256_final(hash_buffer, &sha256_ctx) != TC_CRYPTO_SUCCESS) {
        printk("SHA256 final failed\n");
        return;
    }

    print_sha256_digest(hash_buffer);
}

static void fill_json_packet_from_tracker_payload(const struct sensor_blk *data, char* json_output) {

    time_t raw_time = (time_t)data->time;
    struct tm timeinfo;

    // Convert Unix timestamp to broken-down UTC time (GMT)
    // Use gmtime_r for thread safety, or gmtime if not available
    if (gmtime_r(&raw_time, &timeinfo) == NULL) {
        printk("Failed to convert timestamp\n");
        return;
    }

    int ret = snprintf(json_output, JSON_BUFFER_SIZE,
        JSON_FORMAT,
        timeinfo.tm_year + 1900,
        timeinfo.tm_mon + 1,
        timeinfo.tm_mday,
        timeinfo.tm_hour,
        timeinfo.tm_min,
        timeinfo.tm_sec,
        data->uptime,
        data->lat,
        data->ns,
        data->lon,
        data->ew,
        data->alt,
        data->temp / 100.0,
        data->hum / 100.0,
        data->press / 10.0,
        data->gas / 100.0,
        data->x_accel / 1000.0,
        data->y_accel / 1000.0,
        data->z_accel / 1000.0
    );

    if (ret > 0 && ret < JSON_BUFFER_SIZE) {
        printk("%s\n", json_output);
    } else {
        printk("JSON encoding failed or buffer too small.\n");
        return;
    }
}

/**
 * @brief Helper function to print the SHA-256 hash of a sensor_blk data structure.
 * @param hash_buffer Pointer to a buffer (at least 32 bytes) where the resulting
 *               SHA-256 is stored.
 */
void print_sha256_digest(const uint8_t *hash_buffer) {
    printk("SHA-256 digest: ");
    for (int i = 0; i < TC_SHA256_DIGEST_SIZE; i++) {
        printk("%02x", hash_buffer[i]);
    }
    printk("\n");
}

/**
 * @brief Read callback for the custom characteristic to return packed sensor data.
 *
 * This function called when a connected BLE client reads the value of the
 * custom characteristic. It returns the current contents of the `packed_data` buffer,
 * which containing the tracker sensor data
 *
 * @param conn   Pointer to the connection object.
 * @param attr   Pointer to the GATT attribute being read.
 * @param buf    Buffer to store the read data.
 * @param len    Maximum number of bytes the buffer can hold.
 * @param offset Offset from the beginning of the attribute value.
 *
 * @return Number of bytes read and copied to the buffer, or a negative error code.
 */
static ssize_t pack_data(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                         void *buf, uint16_t len, uint16_t offset) {
    printk("[TRACKER] packing data\n");
    return bt_gatt_attr_read(conn, attr, buf, len, offset, packed_data, PACKED_DATA_LEN);
}

/**
 * @brief Write callback for the custom characteristic to update the device ID.
 *
 * This function is called when a connected BLE client writes to the custom
 * characteristic. It extracts the first byte from the incoming buffer and
 * stores it in the global dev_id variable, representing the device ID.
 *
 * @param conn   Pointer to the connection object.
 * @param attr   Pointer to the GATT attribute being written.
 * @param buf    Pointer to the data buffer provided by the client.
 * @param len    Length of the data being written.
 * @param offset Offset into the attribute value 
 * @param flags  Flags indicating write behavior 
 */
static ssize_t write_handler(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             const void *buf, uint16_t len,
                             uint16_t offset, uint8_t flags) {
    //update device ID
    dev_id = ((uint8_t *)buf)[0];
    // printk("dev_id set to: %d\n", dev_id);

    // Return number of bytes written
    return len;
}


/**
 * @brief Callback triggered when the CCC (Client Characteristic Configuration) is changed.
 *
 * This function is invoked when a BLE client writes to the CCC descriptor of a
 * characteristic to enable or disable notifications. It updates the global
 * notify_enabled flag to indicate.
 *
 * @param attr  Pointer to the CCC GATT attribute.
 * @param value New CCC value set by the client (BT_GATT_CCC_NOTIFY to enable notifications).
 */
static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value) {

    if (value == BT_GATT_CCC_NOTIFY) {
        notify_enabled = true;
        printk("Notifications enabled\n");
    } else {
        printk("Notifications disabled\n");
        notify_enabled = false;
    }
}

/**
 * @brief Bluetooth connection callback when a connection is successfully established.
 *
 * It checks for connection errors, stores a reference to the current connection,
 * prints a status message, and sets the BLE tick flag indicating a BT connection.
 *
 * @param conn Pointer to the new connection object.
 * @param err  Error code (0 if successful, non-zero on failure).
 */
static void connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        printk("[TRACKER] Failed to connect (err %u)\n", err);
        return;
    }

    current_conn = bt_conn_ref(conn);
    printk("[TRACKER] Connected\n");
    ble_tick = 1;
}

/**
 * @brief Bluetooth disconnection callback triggered when the device disconnects.
 *
 * This function is called when the BLE connection is disconnected, prints the
 * disconnection reason, unreferencing, clearing the current connection, and resetting
 * the BLE tick flag.
 *
 * @param conn   Pointer to the disconnected connection object.
 * @param reason Reason code for the disconnection.
 */
static void disconnected(struct bt_conn *conn, uint8_t reason) {
    printk("[TRACKER] Disconnected (reason 0x%02x)\n", reason);
    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }
    ble_tick = 0;
}

/**
 * @brief Helper function to unpack and print contents of packed sensor data.
 */
static void print_packed_data() {
    // Unpack timestamp (uint32_t)
    uint32_t timestamp = (packed_data[32] << 24) |
                         (packed_data[33] << 16) |
                         (packed_data[34] << 8) |
                         packed_data[35];

    // Latitude (int32_t, packed as degrees * 1e7)
    int32_t lat = (packed_data[36] << 24) | (packed_data[37] << 16) |
                  (packed_data[38] << 8)  | packed_data[39];
    char ns = (char)packed_data[40];

    // Longitude (int32_t, packed as degrees * 1e7)
    int32_t lon = (packed_data[41] << 24) | (packed_data[42] << 16) |
                  (packed_data[43] << 8)  | packed_data[44];
    char ew = (char)packed_data[45];

    // Altitude (int16_t, m * 10)
    int16_t alt = (packed_data[46] << 8) | packed_data[47];

    // Satellites
    uint8_t sat = packed_data[48];

    // Temperature (°C × 100)
    int16_t temp = (packed_data[49] << 8) | packed_data[50];

    // Humidity (% × 100)
    int16_t hum  = (packed_data[51] << 8) | packed_data[52];

    // Pressure (hPa × 10)
    int16_t press = (packed_data[53] << 8) | packed_data[54];

    // Gas (arbitrary units × 100)
    int16_t gas = (packed_data[55] << 8) | packed_data[56];

    // Accelerometer X/Y/Z (m/s² × 1000)
    int16_t x = (packed_data[57] << 8) | packed_data[58];
    int16_t y = (packed_data[59] << 8) | packed_data[60];
    int16_t z = (packed_data[61] << 8) | packed_data[62];

    // Device ID (last byte)
    uint8_t dev_id = packed_data[63];

    uint32_t uptime = (packed_data[64] << 24) |
                         (packed_data[65] << 16) |
                         (packed_data[66] << 8) |
                         packed_data[67];

    // Print SHA-256 hash (bytes 0..31)
    printk("[TRACKER] Hash: ");
    for (int i = 0; i < 32; i++) {
        printk("%02X ", packed_data[i]);
    }
    printk("\n");

    // Print unpacked fields
    printk("[TRACKER] Parsed Data:\n");
    printk("  Timestamp: %u\n", timestamp);
    printk("  uptime: %d\n", uptime);
    printk("  Latitude: %.7f %c\n", lat / 1e7, ns);
    printk("  Longitude: %.7f %c\n", lon / 1e7, ew);
    printk("  Altitude: %.1f m\n", alt / 10.0);
    printk("  Satellites: %d\n", sat);
    printk("  Temperature: %.2f °C\n", temp / 100.0);
    printk("  Humidity: %.2f %%\n", hum / 100.0);
    printk("  Pressure: %.1f hPa\n", press / 10.0);
    printk("  Gas: %.2f units\n", gas / 100.0);
    printk("  Accel X: %.3f m/s²\n", x / 1000.0);
    printk("  Accel Y: %.3f m/s²\n", y / 1000.0);
    printk("  Accel Z: %.3f m/s²\n", z / 1000.0);
    printk("  Dev ID: %d\n", dev_id);
}

/**
 * @brief Packs sensor data into a fixed-format byte buffer and sends a BLE notification if enabled.
 * 
 * If notifications are enabled and there is a current BLE connection, a GATT notification
 * containing the packed data is sent on the characteristic.
 * 
 * @param sensor Pointer to the sensor_blk structure containing the sensor data to pack.
 */
extern void pack_sensor_data(const struct sensor_blk *sensor) {
    // Fill bytes 0..31 with SHA-256 digest
    hash_sensor_blk(sensor, packed_data);

    // Timestamp → bytes 32..35
    packed_data[32] = (sensor->time >> 24) & 0xFF;
    packed_data[33] = (sensor->time >> 16) & 0xFF;
    packed_data[34] = (sensor->time >> 8) & 0xFF;
    packed_data[35] = sensor->time & 0xFF;

    // Latitude → bytes 36..39
    int32_t lat_fixed = (int32_t)(sensor->lat * 1e7f);
    packed_data[36] = (lat_fixed >> 24) & 0xFF;
    packed_data[37] = (lat_fixed >> 16) & 0xFF;
    packed_data[38] = (lat_fixed >> 8) & 0xFF;
    packed_data[39] = lat_fixed & 0xFF;
    packed_data[40] = sensor->ns;

    // Longitude → bytes 41..44
    int32_t lon_fixed = (int32_t)(sensor->lon * 1e7f);
    packed_data[41] = (lon_fixed >> 24) & 0xFF;
    packed_data[42] = (lon_fixed >> 16) & 0xFF;
    packed_data[43] = (lon_fixed >> 8) & 0xFF;
    packed_data[44] = lon_fixed & 0xFF;
    packed_data[45] = sensor->ew;

    // Altitude → bytes 46..47
    int16_t alt_fixed = (int16_t)(sensor->alt * 10.0f);
    packed_data[46] = (alt_fixed >> 8) & 0xFF;
    packed_data[47] = alt_fixed & 0xFF;

    //padded byte
    packed_data[48] = 0;


    // Temperature → bytes 49..50
    int16_t temp_fixed = (int16_t)(sensor->temp);
    packed_data[49] = (temp_fixed >> 8) & 0xFF;
    packed_data[50] = temp_fixed & 0xFF;

    // Humidity → bytes 51..52
    int16_t hum_fixed = (int16_t)(sensor->hum);
    packed_data[51] = (hum_fixed >> 8) & 0xFF;
    packed_data[52] = hum_fixed & 0xFF;

    // Pressure → bytes 53..54
    int16_t press_fixed = (int16_t)(sensor->press);
    packed_data[53] = (press_fixed >> 8) & 0xFF;
    packed_data[54] = press_fixed & 0xFF;

    // Gas → bytes 55..56
    int16_t gas_fixed = (int16_t)(sensor->gas);
    packed_data[55] = (gas_fixed >> 8) & 0xFF;
    packed_data[56] = gas_fixed & 0xFF;

    // Accelerometer → bytes 57..62
    int16_t x_fixed = (int16_t)(sensor->x_accel);
    int16_t y_fixed = (int16_t)(sensor->y_accel);
    int16_t z_fixed = (int16_t)(sensor->z_accel);

    packed_data[57] = (x_fixed >> 8) & 0xFF;
    packed_data[58] = x_fixed & 0xFF;
    packed_data[59] = (y_fixed >> 8) & 0xFF;
    packed_data[60] = y_fixed & 0xFF;
    packed_data[61] = (z_fixed >> 8) & 0xFF;
    packed_data[62] = z_fixed & 0xFF;

    // DEV_ID → byte 63
    packed_data[63] = dev_id & 0xFF;

    // Timestamp → bytes 32..35
    packed_data[64] = (sensor->uptime >> 24) & 0xFF;
    packed_data[65] = (sensor->uptime >> 16) & 0xFF;
    packed_data[66] = (sensor->uptime >> 8) & 0xFF;
    packed_data[67] = sensor->uptime & 0xFF;

    // Notify
    if (notify_enabled && current_conn) {
        int err = bt_gatt_notify(current_conn, &custom_svc.attrs[1], packed_data, PACKED_DATA_LEN);
        if (err) {
            printk("[TRACKER] Failed to send notification (err %d)\n", err);
        } else {
            printk("[TRACKER] Notification sent\n");
        }
    }
}

/**
 * @brief Initialize Bluetooth.
 *
 * Enables Bluetooth and registers connection callbacks.
 *
 * @return 0 on success, or a negative error code on failure.
 */
extern int init_bluetooth(void) {
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

/**
 * @brief Start Bluetooth advertising.
 *
 * Begins advertising with predefined parameters and advertising data.
 * Waits briefly after starting to allow advertising packets to propagate.
 *
 * @return 0 on success, or a negative error code on failure.
 */
extern int start_advertising(void) {
    int err;
    printk("[TRACKER] Starting advertising...\n");
    err = bt_le_adv_start(&adv_params, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        printk("[TRACKER] Advertising failed to start (err %d)\n", err);
        return err;
    }

    printk("[TRACKER] Advertising started\n");
    k_msleep(3000);  // Wait before restarting next advertisement cycle ?????
    return 0;
}

/**
 * @brief Stop Bluetooth advertising.
 *
 * Waits briefly to ensure packets clear, then stops advertising.
 *
 * @return 0 on success, or a negative error code on failure.
 */
extern int stop_advertising(void) {
    k_msleep(2000); // wait for packets to clear before stopping
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

/**
 * @brief Stop advertising and disconnect any active connection.
 *
 * Stops advertising, then if a connection exists, disconnects it with
 * a remote user termination reason.
 *
 * @return 0 on success, or a negative error code on failure.
 */
extern int stop_advertising_and_disconnect() {
    int err;

    err = stop_advertising();
    if (err) {
        printk("[TRACKER] Failed to stop advertising\n");
        return err;
    }

    if (current_conn) {
        err = bt_conn_disconnect(current_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        if (err) {
            printk("[TRACKER] Disconnection failed (err %d)\n", err);
            return err;
        } else {
            printk("[TRACKER] Disconnection initiated\n");
        }
    }

    return 0;
}

/**
 * @brief Testing tracker thread sending dummy data
 * 
 */
void tracker_thread(void) {
    int err;

    err = init_bluetooth();
    if (err) {
        printk("[TRACKER] Bluetooth init failed\n");
        return;
    }

    struct tm timeinfo = {
        .tm_year = 2025 - 1900,  // years since 1900
        .tm_mon = 4,             // May (0-indexed: Jan = 0)
        .tm_mday = 25,
        .tm_hour = 12,
        .tm_min = 38,
        .tm_sec = 0,
    };

    uint32_t unix_ts = (uint32_t)timeutil_timegm(&timeinfo);
    printk("Unix timestamp: %u\n", unix_ts);

    // Example sensor data
    struct sensor_blk sensor = {
        .time = unix_ts,
        .uptime = 5000,
        .lat = 37.7699999f,
        .ns = 'N',
        .lon = -122.419998f,
        .ew = 'W',
        .alt = 12.3f,
        .temp = 2350, //23.50
        .hum = 4567, //45.67
        .press = 10132, //10113.2
        .gas = 5230, //52.30
        .x_accel = -900, //-0.900,
        .y_accel = 200, //0.200,
        .z_accel = 985 //0.985
    };

    // uint8_t digest[TC_SHA256_DIGEST_SIZE];
    // hash_sensor_blk(&sensor, digest);

    // print_sha256_digest(digest);

    while (1) {
        err = start_advertising();
        if (err) {
            return;
        }
        // Advertise sensor data 5 times, once per second
        for (int i = 0; i < 5; i++) {
            pack_sensor_data(&sensor);
            // print_packed_data();

            // Simulate sensor updates
            sensor.temp += 0.1;
            sensor.hum += 0.1;
            sensor.press += 0.1;
        }

        err = stop_advertising_and_disconnect();
        if (err) {
            return;
        }
        k_sleep(K_SECONDS(10));  // Wait before restarting next advertisement cycle
    }
}

//MAIN.C bluetooth test
// #include <zephyr/kernel.h>
// #include <zephyr/types.h>
// #include <zephyr/sys/printk.h>
// #include <zephyr/logging/log.h>
// #include <zephyr/net_buf.h>
// #include <zephyr/bluetooth/bluetooth.h>
// #include <zephyr/bluetooth/conn.h>
// #include <zephyr/bluetooth/gatt.h>
// #include <zephyr/bluetooth/hci.h>

// #include <tinycrypt/sha256.h>
// #include <tinycrypt/constants.h>
// #include <string.h>

// #include <bluetooth.h>

// K_THREAD_DEFINE(tracker_tid, TRACKER_CONTROL_STACK_SIZE, tracker_thread, NULL, NULL, NULL, TRACKER_CONTROL_PRIORITY, 0, 0);
