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
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/drivers/gpio/gpio_sx1509b.h>
#include <zephyr/pm/pm.h>
#include <drivers/gnss/ublox_neo_m9n.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/pm/device.h>
#include <inttypes.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>


#define MAX_SAMPLES 1
#define TEST_PARTITION	storage_partition
#define TEST_PARTITION_OFFSET FIXED_PARTITION_OFFSET(TEST_PARTITION)
#define TEST_PARTITION_DEVICE FIXED_PARTITION_DEVICE(TEST_PARTITION)

#define FLASH_PAGE_SIZE   4096

#define NEO_SERIAL DT_NODELABEL(neom9n)

#define LED_MAX 255
#define LED_OFF 0
#define NUMBER_OF_LEDS 3
#define RED_LED DT_GPIO_PIN(DT_NODELABEL(led0), gpios)
#define GREEN_LED DT_GPIO_PIN(DT_NODELABEL(led1), gpios)
#define BLUE_LED DT_GPIO_PIN(DT_NODELABEL(led2), gpios)

static const gpio_pin_t rgb_pins[] = {
	RED_LED,
	GREEN_LED,
	BLUE_LED,
};

const struct device *sx1509b_dev;

#define ALARM_CHANNEL_ID 0
#define UPDATE 10// Update window (interrupts)
#define SAMPLE_RATE 500 // Time between samples grabbed in a row while awake
#define X_THRESHOLD 2
#define Y_THRESHOLD 2
#define Z_THRESHOLD 10
#define LIS2DH12_NODE DT_NODELABEL(lis2dh12)

LOG_MODULE_REGISTER(tracker, LOG_LEVEL_INF);

// #define LIS2DH_REG_CTRL2 0x19
// static const struct i2c_dt_spec accelerometer_i2c_spec = I2C_DT_SPEC_GET(DT_ALIAS(accel0));

const struct device *const rtc_2 = DEVICE_DT_GET(DT_ALIAS(rtc2));
struct counter_top_cfg ctr_top;

int rtc_tick = 0;
int accel_tick = 0;
int ble_tick = 0;

struct sensor_blk {
    uint32_t time;
    int64_t uptime;
    float lat;
    char ns;
    float lon;
    char ew;
    float alt;
    int16_t temp;
    int16_t hum;
    int16_t press;
    int16_t gas;
    int16_t x_accel; 
    int16_t y_accel;
    int16_t z_accel;
};


static struct neom9n_api *neo_api;

const struct device *neo_dev;
const struct device *pressure;
const struct device *humidity;
const struct device *evoc;
const struct device *accel;


void rtc_interrupt(const struct device *dev, void *user_data) {
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);
    rtc_tick = 1;
}

void init_rtc(void) {
    ctr_top.ticks = counter_us_to_ticks(rtc_2, UPDATE * USEC_PER_SEC);
    ctr_top.user_data = &ctr_top;
    ctr_top.callback = rtc_interrupt;
    ctr_top.flags = 0;
    counter_set_top_value(rtc_2, &ctr_top);
    counter_start(rtc_2);
}


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
    return 0;
}

int read_gnss(uint32_t *time, 
    float* lat, char *ns, float *lon, char *ew, float *alt, int *sat) {
    int rc = neo_api->fetch_data(neo_dev);
    if (rc) {
        LOG_WRN("error fetching gnss data.\n");
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
        LOG_WRN("%f     %f       %f      %d", *lat, *lon, *alt, *sat);
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

// TODO: trigger from ble connect 
void read_loop(const struct device *flash_dev, uint8_t write_block_size) {
    // Triggered from bluetooth wakeup
    uint8_t size = 0;
     uint32_t read_size = 0;
    uint32_t offset = FLASH_PAGE_SIZE + TEST_PARTITION_OFFSET;
    flash_read(flash_dev, TEST_PARTITION_OFFSET, &size, write_block_size);
    flash_read(flash_dev, TEST_PARTITION_OFFSET + write_block_size, &read_size, write_block_size);
    struct sensor_blk sensors;
    offset = FLASH_PAGE_SIZE + TEST_PARTITION_OFFSET + (read_size * sizeof(struct sensor_blk));
    // Unpacks data since last bluetooth grab
    // Im blue dabadeebadubah
    for (int i = read_size; i < size; i++) {

        // read out single struct of data
        flash_read(flash_dev, offset, &sensors, sizeof(sensors));
        offset += sizeof(sensors);
        read_size++;
        //LOG_INF("read: %d     %d      %d       %d        %d       %d        %d\n", 
        //    sensors.temp, sensors.hum, sensors.press, sensors.gas, sensors.x_accel, sensors.y_accel, sensors.z_accel);
        //LOG_INF("gps read: %f      %f      %f", sensors.lat, sensors.lon, sensors.alt);
        // TODO: send sensors struct over bluetooth

    }

    //set the new "latest read offset"
    flash_erase(flash_dev, TEST_PARTITION_OFFSET, FLASH_PAGE_SIZE);
    if (flash_write(flash_dev, TEST_PARTITION_OFFSET, &size, write_block_size)) {
         LOG_WRN("Size not updated\n");   
    }
    if (flash_write(flash_dev, TEST_PARTITION_OFFSET + write_block_size, &read_size, write_block_size)) {
        LOG_WRN("Read size was not updated!\n");
    }
}

void write_loop(const struct device *flash_dev, uint8_t write_block_size) {
    struct sensor_blk sensors;
    int sat;
    int err;
    uint32_t size = 0;
    uint8_t byte_array[sizeof(struct sensor_blk)];
    uint8_t write_cycles = sizeof(sensors) / write_block_size;
    flash_read(flash_dev, TEST_PARTITION_OFFSET, &size, sizeof(uint32_t));
    uint32_t offset = FLASH_PAGE_SIZE + TEST_PARTITION_OFFSET + ((size) * (sizeof(sensors)));
    // Im red like roses
    sensors.lat = 1;
    sensors.ns = 1;
    sensors.lon = 1;
    sensors.ew = 1;
    sensors.alt = 1;
    uint8_t retry = 0;
    for (int i = 0; i < MAX_SAMPLES; i++) {
        // Sleep for some time before grabbing more data
        k_msleep(SAMPLE_RATE);
        err = read_sensors(&(sensors.temp), &(sensors.hum), &(sensors.press), &(sensors.gas), &(sensors.x_accel), &(sensors.y_accel), &(sensors.z_accel));
        if (err) {
            LOG_WRN("Sensor readings invalid.\n");
            sensors.temp = 0;
            sensors.hum = 0;
            sensors.press = 0;
            sensors.gas = 0;
            sensors.x_accel = 0;
            sensors.y_accel = 0;
            sensors.z_accel = 0;
        }
read_data:
        err |= read_gnss(&(sensors.time), &(sensors.lat), &(sensors.ns), &(sensors.lon), &(sensors.ew), &(sensors.alt), &sat);
        if (err) {
            retry++;
            if (retry > 5) {
                // Attempted to gather data several times and was unsuccesful
                LOG_WRN("Too many incorrect attempts at reading gnss.\n");
                sensors.lat = 0;
                sensors.lon = 0;
                sensors.alt = 0;
                sensors.time = 0;
            } else {
                goto read_data;
            }
        }

        sensors.uptime = k_uptime_get();
        memcpy(&byte_array, &(sensors), sizeof(sensors));
        //LOG_INF("GPS: %f      %f      %f \n", sensors.lat, sensors.lon, sensors.alt);
        //LOG_INF("write: %d     %d      %d       %d        %d       %d        %d\n", 
        //    sensors.temp, sensors.hum, sensors.press, sensors.gas, sensors.x_accel, sensors.y_accel, sensors.z_accel);
        for (int j = 0; j < write_cycles; j++) {
            offset = FLASH_PAGE_SIZE + TEST_PARTITION_OFFSET + ((size) * (sizeof(sensors))) + j * write_block_size;
            flash_write(flash_dev, offset, &byte_array[j * write_block_size], write_block_size);
        }
        size++;
    }
    uint32_t read_size;
    flash_read(flash_dev, TEST_PARTITION_OFFSET + write_block_size, &read_size, write_block_size);
    flash_erase(flash_dev, TEST_PARTITION_OFFSET, FLASH_PAGE_SIZE);
    if (flash_write(flash_dev, TEST_PARTITION_OFFSET, &size, write_block_size)) {
         LOG_WRN("Size not updated");   
    }
    if (flash_write(flash_dev, TEST_PARTITION_OFFSET + write_block_size, &read_size, write_block_size)) {
        LOG_WRN("Last read offset was not updated!\n");
    }
    return;
}

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
        LOG_WRN("Rapid motion detected!\n)");
        accel_tick = 1;
    }
}

int main(void) { 
    const struct device *flash_dev = TEST_PARTITION_DEVICE;
    struct flash_parameters flash_params;
    memcpy(&flash_params, flash_get_parameters(flash_dev), sizeof(flash_params));
    uint8_t write_block_size = flash_params.write_block_size;

    int err = 0;
    const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(cons)) {
		printf("%s: device not ready.\n", cons->name);
		return 0;
	}

    if (!device_is_ready(flash_dev)) {
		printf("Internal storage device not ready\n");
        return EINVAL;
	}
    init_rtc();
    sx1509b_dev = DEVICE_DT_GET(DT_NODELABEL(sx1509b));
    // Setup led
	if (!device_is_ready(sx1509b_dev)) {
		LOG_WRN("sx1509b: device not ready.\n");
	}
    for (int i = 0; i < NUMBER_OF_LEDS; i++) {
		err = sx1509b_led_intensity_pin_configure(sx1509b_dev,
							  rgb_pins[i]);

		if (err) {
			LOG_DBG("Error configuring pin for LED intensity\n");
		}
    }
    neo_dev = DEVICE_DT_GET(NEO_SERIAL);
    pressure = DEVICE_DT_GET_ONE(st_lps22hb_press);
    humidity = DEVICE_DT_GET_ONE(st_hts221);
    evoc = DEVICE_DT_GET_ONE(ams_ccs811);
    accel = DEVICE_DT_GET(DT_ALIAS(accel0));
    err = init_sensors();
    err |= init_gnss();
    if (err) {
        LOG_WRN("error setting up devices. Shutting down.\n");
        return err;
    }
    // NOTE: This trigger is set on data ready because the thingy52 does not 
    // connect the Int2 pin (responsible for Delta and Threshold triggers) 
    // of the lis2dh12 to any internal gpio pins. 

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

    uint32_t size = 0;
    uint32_t read_size = 0;
    LOG_INF( "%u\n", sizeof(struct sensor_blk));
    flash_erase(flash_dev, TEST_PARTITION_OFFSET, FLASH_PAGE_SIZE * 6);
    flash_write(flash_dev, TEST_PARTITION_OFFSET, &size, write_block_size);
    flash_write(flash_dev, TEST_PARTITION_OFFSET + write_block_size, &read_size, write_block_size);
    rtc_tick = 1;

    // Accuatoor: red on rtc, green on accel, blue on ble
    while(1) {  
        // Loop occurs on every wakeup from idle or after every occurance
        if (rtc_tick) {
            // Woke from rtc
            sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, LED_MAX);
            sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, LED_OFF);
            sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, LED_OFF);
  
            write_loop(flash_dev, write_block_size);
            rtc_tick = 0;
            //read_loop(flash_dev, write_block_size);
            sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, LED_OFF);
            sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, LED_OFF);
            sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, LED_OFF);
            // TODO: LAST THING TO DO!!! check GPS coords is close to a base node.

            // TODO: Start advertising and set ble_tick

        } else if (accel_tick) {
            // Woke from accel
            sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, LED_OFF);
            sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, LED_OFF);
            sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, LED_MAX);
            write_loop(flash_dev, write_block_size);
            accel_tick = 0;
            sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, LED_OFF);
            sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, LED_OFF);
            sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, LED_OFF);
        } 
        if (ble_tick) {
            // TODO: Woke from BLE
            sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, LED_OFF);
            sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, LED_MAX);
            sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, LED_OFF);
            read_loop(flash_dev, write_block_size);
            ble_tick = 0;
            sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, LED_OFF);
            sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, LED_OFF);
            sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, LED_OFF);
            // TODO: Stop advertising and disconnect. 
        }
        LOG_INF("Napping... zzz...\n");
        // epilepsy check begin shutdown process
        sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, LED_MAX);
        sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, LED_MAX);
        sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, LED_MAX);
        k_msleep(1000);
        sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, LED_OFF);
        sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, LED_OFF);
        sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, LED_OFF);
        while (!rtc_tick && !accel_tick && !ble_tick) {
            k_cpu_idle(); // truly idle CPU while waiting
        }
    }
}