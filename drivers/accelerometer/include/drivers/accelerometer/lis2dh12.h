#ifndef LIS2DH12_H
#define LIS2DH12_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>
#include <zephyr/data/json.h>
#include <stdlib.h>
#include <zephyr/sys/printk.h>

int init_lis2dh12(struct device *lis2dh12);

int fetch_lis2dh12(struct device *lis2dh12, double *x, double *y, double *z);

#endif