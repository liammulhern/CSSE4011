#include <drivers/humidity/hts221.h>

int init_hts221(struct device *st_hts221) {
    if (!device_is_ready(st_hts221)) { 
        return EINVAL;
    }
    return 0;
}

int fetch_hts221(struct device *st_hts221, double *humidity) {
    double hum_raw;
    if (sensor_sample_fetch(st_hts221) < 0) {
        // Sample update error
        return -1;
    }

    if (sensor_channel_get(st_hts221, SENSOR_CHAN_HUMIDITY, &hum_raw) < 0) {
        // pressure channel reading error
        return -2;
    }

    *humidity = sensor_value_to_double(&hum_raw);
    return 0;
}
