#include <drivers/accelerometer/lis2dh12.h>

int init_lis2dh12(struct device *lis2dh12) {
    if (!device_is_ready(lis2dh12)) { 
        return EINVAL;
    }
    return 0;
}

int fetch_lis2dh12(struct device *lis2dh12, double *x, double *y, double *z) {
    struct sensor_value x_raw;
    struct sensor_value y_raw;
    struct sensor_value z_raw;

    if (sensor_sample_fetch(lis2dh12) < 0) {
        LOG_ERR("Failed to fetch LIS2DH12 sample");
        return -EIO;
    }

    if (sensor_channel_get(lis2dh12, SENSOR_CHAN_ACCEL_X, &x_raw) < 0 ||
        sensor_channel_get(lis2dh12, SENSOR_CHAN_ACCEL_Y, &y_raw) < 0 ||
        sensor_channel_get(lis2dh12, SENSOR_CHAN_ACCEL_Z, &z_raw) < 0) {
        LOG_ERR("Failed to read LIS2DH12 sensor channel");
        return -EIO;
    }

    *x = sensor_value_to_double(&x_raw);
    *y = sensor_value_to_double(&y_raw);
    *z = sensor_value_to_double(&z_raw);
    return 0;
}