#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/util.h>
#include <zephyr/data/json.h>
#include <stdlib.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include <drivers/gnss/ublox_neo_m9n.h>


#define TEST_PARTITION	storage_partition
#define TEST_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(TEST_PARTITION)
#define TEST_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(TEST_PARTITION)

#define FLASH_PAGE_SIZE   4096

#define NEO_SERIAL DT_NODELABEL(neom9n)

#if defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE) &&	\
	defined(CONFIG_FLASH_HAS_NO_EXPLICIT_ERASE)
#define FLASH_PE_RUNTIME_CHECK(cond) (cond)
#elif defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE)
#define FLASH_PE_RUNTIME_CHECK(cond) (true)
#else
#define FLASH_PE_RUNTIME_CHECK(cond) (false)
#endif

struct meta_data {
    uint32_t header;
    size_t len_blks; 
};

struct sensor_blk {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    float lat;
    char ns;
    float lon;
    char ew;
    float alt; 
    int sat;
    double temp;
    double hum;
    double press;
    double gas;
    double x_accel; 
    double y_accel;
    double z_accel;
};


static struct neom9n_api *neo_api;

const struct device *neo_dev;
const struct device *pressure;
const struct device *humidity;
const struct device *evoc;
const struct device *accel;
/*
* Returns 0 if exited correctly
*/
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

int init_gnss(void) { 

    if (!device_is_ready(neo_dev)) {
        return -EINVAL;
    }
    neo_api = (struct neom9n_api *) neo_dev->api;

    neo_api->cfg_nav5(neo_dev, Stationary, P_2D, 0, 1, 5, 100, 100, 100, 350, 0, 60, 0, 0, 0,
              AutoUTC);
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
    neo_api->cfg_msg(neo_dev, NMEA_ZDA, 0);
}

void read_gnss(struct time *neotime, 
    float* lat, char *ns, float *lon, char *ew, float *alt, int *sat) {

            neo_api->get_time(neo_dev, neotime);
            neo_api->get_latitude(neo_dev, lat);
            neo_api->get_ns(neo_dev, ns);
            neo_api->get_longitude(neo_dev, lon);
            neo_api->get_ew(neo_dev, ew);
            neo_api->get_altitude(neo_dev, alt);
            neo_api->get_satellites(neo_dev, sat);
}

void read_sensors(double *temp, double *hum, double *press, 
    double *gas, double *x_accel, double *y_accel, double *z_accel) {
    
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
    if (sensor_channel_get(pressure, SENSOR_CHAN_PRESS, &temp_raw) < 0) {
        // pressure channel reading error
        return -EIO;
    }
    if (sensor_channel_get(pressure, SENSOR_CHAN_AMBIENT_TEMP, &pressure_raw) < 0) {
        // temperature channel reading error
        return -EIO;
    }
    if (sensor_channel_get(evoc, SENSOR_CHAN_VOC, &gas) < 0) {
        // pressure channel reading error
        return -EIO;
    }

    if (sensor_channel_get(accel, SENSOR_CHAN_ACCEL_X, &x_raw) < 0 ||
        sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Y, &y_raw) < 0 ||
        sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Z, &z_raw) < 0) {
        return -EIO;
    }

    *x_accel = sensor_value_to_double(&x_raw);
    *y_accel = sensor_value_to_double(&y_raw);
    *z_accel = sensor_value_to_double(&z_raw);

    *hum = sensor_value_to_double(&hum_raw);
    *temp = sensor_value_to_double(&temp_raw);
    *press = sensor_value_to_double(&pressure_raw);
    *gas = sensor_value_to_double(&gas_raw);
}


void write_to_flash(struct device *flash_dev, struct sensor_blk sensors, int size, off_t offset) { 
    flash_write(flash_dev, 0, &size, sizeof(size));
    flash_write(flash_dev, offset, &sensors, sizeof(sensors));
}


int read_loop(struct device *flash_dev) {
    // Trigger this by bluetooth callback
    int size = 0;
    off_t offset = 0 + sizeof(int);
    flash_read(flash_dev, 0, &size, sizeof(int));
    struct sensor_blk sensors;
    for (int i = 0; i < size; i++) {

        // read out structs of data, pack into msgq
        flash_read(flash_dev, offset, &sensors, sizeof(sensors));
        offset += sizeof(sensors);

        // pack msgq
    }
}


int write_loop(void) {

    const struct device *flash_dev = TEST_PARTITION_DEVICE;
    struct flash_parameters flash_params;
    memcpy(&flash_params, flash_get_parameters(flash_dev), sizeof(flash_params));

    if (!device_is_ready(flash_dev)) {
		printf("Internal storage device not ready\n");
        return EINVAL;
	}

    neo_dev = DEVICE_DT_GET(NEO_SERIAL);
    pressure = DEVICE_DT_GET_ONE(st_lps22hb_press);
    humidity = DEVICE_DT_GET_ONE(st_hts221);
    evoc = DEVICE_DT_GET_ONE(ams_ccs811);
    accel = DEVICE_DT_GET_ONE(st_lis2dh12);
    struct time neotime;
    int sat;
    struct sensor_blk sensors;
    int size = 0;
    off_t offset = 0 + sizeof(int);
    int err = 0;
    err = init_sensors();
    err = init_gnss();
    if (err) {
        return err;
    }

    while (1) {
        // wake up;
        k_msleep(2000);

        read_gnss(&neotime, &(sensors.lat), &(sensors.ns), &(sensors.lon), &(sensors.ew), &(sensors.alt), &sat);
        read_sensors(&(sensors.temp), &(sensors.hum), &(sensors.press), &(sensors.gas), &(sensors.x_accel), &(sensors.y_accel), &(sensors.z_accel));
        sensors.hour = neotime.hour;
        sensors.minute = neotime.minute;
        sensors.second = neotime.second;
        write_to_flash(flash_dev, sensors, size, offset);
        flash_write(flash_dev, offset, (uint8_t *) &sensors, sizeof(sensors));
        offset += sizeof(sensors);
        size++;
        // Wrtie to memory
    }
    return err;
}

int main(void) { 
    const struct device *flash_dev = TEST_PARTITION_DEVICE;
    struct flash_parameters flash_params;
    memcpy(&flash_params, flash_get_parameters(flash_dev), sizeof(flash_params));

    if (!device_is_ready(flash_dev)) {
		printf("Internal storage device not ready\n");
        return EINVAL;
	}

    neo_dev = DEVICE_DT_GET(NEO_SERIAL);
    pressure = DEVICE_DT_GET_ONE(st_lps22hb_press);
    humidity = DEVICE_DT_GET_ONE(st_hts221);
    evoc = DEVICE_DT_GET_ONE(ams_ccs811);
    accel = DEVICE_DT_GET_ONE(st_lis2dh12);
    struct time neotime;
    int sat;
    struct sensor_blk sensors;
    int size = 0;
    off_t offset = 0 + sizeof(int);
    int err = 0;
    err = init_sensors();
    err = init_gnss();
    if (err) {
        return err;
    }
    // IDK IF I NEED TO MAKE A GOTO STATEMENT FOR THIS ON EVERY WAKEUP OR IF IT RUNS NORMALLY.
}