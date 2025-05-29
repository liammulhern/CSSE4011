#include <sensors.h>


const struct device *neo_dev;
static struct neom9n_api *neo_api;
const struct device *pressure;
const struct device *humidity;
const struct device *evoc;
const struct device *accel;

uint8_t accel_tick = 0;

// uint8_t get_accel_tick(void) {
//     return accel_tick;
// }

// void set_accel_tick(int set) {
//     accel_tick = set;
// }

static void accel_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	// Reached when threshold reach 
    struct sensor_value accel_raw[3];
    sensor_sample_fetch(dev);
    sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accel_raw);
    double x = sensor_value_to_double(&accel_raw[0]);
    double y =  sensor_value_to_double(&accel_raw[1]);
    double z =  sensor_value_to_double(&accel_raw[2]);
    if ((x > X_THRESHOLD) || (y > Y_THRESHOLD) || (z > Z_THRESHOLD)) {
        accel_tick = 1;
    }
}

void bind_sensors(void) {
    neo_dev = DEVICE_DT_GET(NEO_SERIAL);
    pressure = DEVICE_DT_GET_ONE(st_lps22hb_press);
    humidity = DEVICE_DT_GET_ONE(st_hts221);
    evoc = DEVICE_DT_GET_ONE(ams_ccs811);
    accel = DEVICE_DT_GET(DT_ALIAS(accel0));
}

int init_anymotion(void) {
    // NOTE: This trigger is set on data ready because the thingy52 does not 
    // connect the Int2 pin (responsible for Delta and Threshold triggers) 
    // of the lis2dh12 to any internal gpio pins. 
    int err = 0;
    struct sensor_trigger trig;
    trig.type = SENSOR_TRIG_DATA_READY;
    trig.chan = SENSOR_CHAN_ACCEL_XYZ;

    struct sensor_value odr = {
                .val1 = 1,
    };

    err = sensor_attr_set(accel, trig.chan,
                            SENSOR_ATTR_SAMPLING_FREQUENCY,
                            &odr);
    err = sensor_trigger_set(accel, &trig, accel_handler);
    return err;
    
}


int init_gnss(void) { 

    if (!device_is_ready(neo_dev)) {
        return -EINVAL;
    }
    neo_api = (struct neom9n_api *) neo_dev->api;

    neo_api->cfg_nav5(neo_dev, Portable, P_3D, 0, 1, 5, 100, 100, 100, 350, 0, 60, 0, 0, 0,
              AutoUTC); // Portable changed from Stationary UNSURE :TODO:
    neo_api->cfg_gnss(neo_dev, 0, 32, 5, 0, 8, 16, 0, 0x01010001, 1, 1, 3, 0, 0x01010001, 3, 8,
              16, 0, 0x01010000, 5, 0, 3, 0, 0x01010001, 6, 8, 14, 0, 0x01010001);
    neo_api->cfg_msg(neo_dev, NMEA_DTM, 0);
    neo_api->cfg_msg(neo_dev, NMEA_GBQ, 0);
    neo_api->cfg_msg(neo_dev, NMEA_GBS, 0);
    neo_api->cfg_msg(neo_dev, NMEA_GLL, 0);
    neo_api->cfg_msg(neo_dev, NMEA_GLQ, 0);
    neo_api->cfg_msg(neo_dev, NMEA_GNQ, 0);
    neo_api->cfg_msg(neo_dev, NMEA_GNS, 0);
    neo_api->cfg_msg(neo_dev, NMEA_GPQ, 0);
    neo_api->cfg_msg(neo_dev, NMEA_GRS, 0);
    neo_api->cfg_msg(neo_dev, NMEA_GSA, 0);
    neo_api->cfg_msg(neo_dev, NMEA_GST, 0);
    neo_api->cfg_msg(neo_dev, NMEA_GSV, 0);
    neo_api->cfg_msg(neo_dev, NMEA_RMC, 0);
    neo_api->cfg_msg(neo_dev, NMEA_THS, 0);
    neo_api->cfg_msg(neo_dev, NMEA_TXT, 0);
    neo_api->cfg_msg(neo_dev, NMEA_VLW, 0);
    neo_api->cfg_msg(neo_dev, NMEA_VTG, 0);
    neo_api->cfg_msg(neo_dev, NMEA_ZDA, 1);
    return 0;
}

int read_gnss(uint32_t *time, 
    float* lat, char *ns, float *lon, char *ew, float *alt, int *sat) {
    int rc = neo_api->fetch_data(neo_dev);
    if (rc) {
        return -rc;
    }
    neo_api->get_timestamp(neo_dev, time);
    neo_api->get_latitude(neo_dev, lat);
    neo_api->get_ns(neo_dev, ns);
    neo_api->get_longitude(neo_dev, lon);
    neo_api->get_ew(neo_dev, ew);
    neo_api->get_altitude(neo_dev, alt);
    neo_api->get_satellites(neo_dev, sat);

    if (*sat == 0) {
        return -EINVAL;
    }
    return 0;
}


int init_sensors(void) {

    

    if (!device_is_ready(pressure)) { 
        return -EINVAL;
    }
    if (!device_is_ready(humidity)) { 
        return -EINVAL;
    }
    if (!device_is_ready(evoc)) { 
        return -EINVAL;
    }
    if (!device_is_ready(accel)) { 
        return -EINVAL;
    }
    return 0;
}

int read_sensors(int16_t *temp, int16_t *hum, int16_t *press, 
    int16_t *gas, int16_t *x_accel, int16_t *y_accel, int16_t *z_accel) {
    
    struct sensor_value gas_raw;
    struct sensor_value temp_raw;
    struct sensor_value pressure_raw;
    struct sensor_value hum_raw;
    struct sensor_value x_raw;
    struct sensor_value y_raw;
    struct sensor_value z_raw;
    if (sensor_sample_fetch(humidity) < 0) {
        // Sample update error
        return -EIO;
    }
    if (sensor_sample_fetch(pressure) < 0) {
        // Sample update error
        return -EIO;
    }
    if (sensor_sample_fetch(evoc) < 0) {
        // Sample update error
        return -EIO;
    }
    if (sensor_sample_fetch(accel) < 0) {
        // Sample update error
        return -EIO;
    }

    if (sensor_channel_get(humidity, SENSOR_CHAN_HUMIDITY, &hum_raw) < 0) {
        // humidity channel reading error
        return -EIO;
    }
    if (sensor_channel_get(pressure, SENSOR_CHAN_PRESS, &pressure_raw) < 0) {
        // pressure channel reading error
        return -EIO;
    }
    if (sensor_channel_get(pressure, SENSOR_CHAN_AMBIENT_TEMP, &temp_raw) < 0) {
        // temperature channel reading error
        return -EIO;
    }
    if (sensor_channel_get(evoc, SENSOR_CHAN_VOC, &gas_raw) < 0) {
        // pressure channel reading error
        return -EIO;
    }

    if (sensor_channel_get(accel, SENSOR_CHAN_ACCEL_X, &x_raw) < 0 ||
        sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Y, &y_raw) < 0 ||
        sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Z, &z_raw) < 0) {
        return -EIO;
    }

    double temp_x_accel = sensor_value_to_double(&x_raw);
    double temp_y_accel = sensor_value_to_double(&y_raw);
    double temp_z_accel = sensor_value_to_double(&z_raw);
    double temp_hum = sensor_value_to_double(&hum_raw);
    double temp_temp = sensor_value_to_double(&temp_raw);
    double temp_press = sensor_value_to_double(&pressure_raw);
    double temp_gas = sensor_value_to_double(&gas_raw);

    *temp = (int16_t)(temp_temp * 100);
    *hum = (int16_t)(temp_hum * 100);
    *press = (int16_t)(temp_press * 10);
    *gas = (int16_t)(temp_gas * 100);
    *x_accel = (int16_t)(temp_x_accel * 1000.0);
    *y_accel = (int16_t)(temp_y_accel * 1000.0);
    *z_accel = (int16_t)(temp_z_accel * 1000.0);
 
    return 0;
}
