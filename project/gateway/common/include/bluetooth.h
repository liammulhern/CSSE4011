#ifndef BASE_SERVICE_H
#define BASE_SERVICE_H

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>

#include <bluetooth.h>

#define BASE_CONTROL_STACK_SIZE 2048
#define BASE_CONTROL_PRIORITY 5

extern void base_thread(void);

#endif // BASE_SERVICE_H