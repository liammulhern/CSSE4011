#ifndef HTS221_H
#define HTS221_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>
#include <zephyr/data/json.h>
#include <stdlib.h>
#include <zephyr/sys/printk.h>

int init_hts221(struct device *st_hts221);

int fetch_hts221(struct device *st_hts221, double *humidity);

#endif
