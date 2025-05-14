#include <drivers/temp/lps22hb.h>

int init_lps22hb(struct device *st_lps22hb) {
    if (!device_is_ready(st_lps22hb)) { 
        return EINVAL;
    }
    return 0;
}

int fetch_lps22hb(struct device *st_lps22hb, double* temp, double* pressure) {
    double temp_raw;
    double pressure_raw;
    if (sensor_sample_fetch(st_lps22hb) < 0) {
        // Sample update error
        return -1;
    }

    if (sensor_channel_get(st_lps22hb, SENSOR_CHAN_PRESS, &temp_raw) < 0) {
        // pressure channel reading error
        return -2;
    }

    if (sensor_channel_get(st_lps22hb, SENSOR_CHAN_AMBIENT_TEMP, &pressure_raw) < 0) {
        // temperature channel reading error
        return -3;
    }

    *temp = sensor_value_to_double(&temp_raw);
    *pressure = sensor_value_to_double(&pressure_raw);
    return 0;
}
