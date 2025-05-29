#ifndef TRACKER_SENSORS_H
#define TRACKER_SENSORS_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>

#include <zephyr/sys/util.h>
#include <stdlib.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_sx1509b.h>
#include <drivers/gnss/ublox_neo_m9n.h>
#include <zephyr/drivers/hwinfo.h>
#include <inttypes.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#include <flash.h>

#define X_THRESHOLD 2
#define Y_THRESHOLD 2
#define Z_THRESHOLD 10
#define LIS2DH12_NODE DT_NODELABEL(lis2dh12)
#define NEO_SERIAL DT_NODELABEL(neom9n)


extern const struct device *neo_dev;
extern const struct device *pressure;
extern const struct device *humidity;
extern const struct device *evoc;
extern const struct device *accel;


uint8_t get_accel_tick(void);

void set_accel_tick(int set);

void bind_sensors(void);


int init_anymotion(void);

int init_gnss(void);

int read_gnss(uint32_t *time, 
    float* lat, char *ns, float *lon, char *ew, float *alt, int *sat);

int init_sensors(void);

int read_sensors(int16_t *temp, int16_t *hum, int16_t *press, 
    int16_t *gas, int16_t *x_accel, int16_t *y_accel, int16_t *z_accel);

#endif