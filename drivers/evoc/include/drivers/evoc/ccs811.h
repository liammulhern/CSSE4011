#ifndef CCS811_H
#define CCS811_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>
#include <zephyr/data/json.h>
#include <stdlib.h>
#include <zephyr/sys/printk.h>

int init_ccs811(struct device *ams_ccs811);

int fetch_ccs811(struct device *ams_ccs811, double* gas);

#endif
