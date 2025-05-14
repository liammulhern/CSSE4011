#include <drivers/evoc/ccs811.h>.

int init_ccs811(struct device *ams_811) {
    if (!device_is_ready(ams_811)) { 
        return EINVAL;
    }
    return 0;
}

int fetch_ccs811(struct device *ams_ccs811, double* gas) {
    double gas_raw;
    if (sensor_sample_fetch(ams_ccs811) < 0) {
        // Sample update error
        return -1;
    }

    if (sensor_channel_get(ams_ccs811, SENSOR_CHAN_VOC, &gas) < 0) {
        // pressure channel reading error
        return -2;
    }

    *gas = sensor_value_to_double(&gas_raw);
    return 0;
}
