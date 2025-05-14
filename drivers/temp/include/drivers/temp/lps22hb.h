#ifndef LPS22HB_H
#define LPS22HB_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>
#include <zephyr/data/json.h>
#include <stdlib.h>
#include <zephyr/sys/printk.h>

int init_lps22hb(struct device *st_lps22hb);

int fetch_lps22hb(struct device *st_lps22hb, double* temp, double* pressure);

#endif
