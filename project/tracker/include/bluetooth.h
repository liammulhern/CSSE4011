#ifndef TRACKER_SERVICE_H
#define TRACKER_SERVICE_H

#include <zephyr/types.h>
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

#include <sensors.h>


#define TRACKER_CONTROL_STACK_SIZE 2048
#define TRACKER_CONTROL_PRIORITY 5


extern uint8_t get_ble_tick(void);

extern void set_ble_tick(uint8_t value);

extern void pack_sensor_data(const struct sensor_blk *sensor);

extern int init_bluetooth(void);

extern int start_advertising(void);

extern int stop_advertising(void);

extern int stop_advertising_and_disconnect();

extern void tracker_thread(void);

#endif // TRACKER_SERVICE_H