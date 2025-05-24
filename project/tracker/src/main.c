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
#define UPDATE 10000000// Update window (interrupts)
#define SAMPLE_RATE 500 // Time between samples grabbed in a row while awake

#define LIS2DH12_NODE DT_NODELABEL(lis2dh12)

LOG_MODULE_REGISTER(tracker, LOG_LEVEL_INF);

#define LIS2DH_REG_CTRL2 0x19
static const struct i2c_dt_spec accelerometer_i2c_spec = I2C_DT_SPEC_GET(DT_ALIAS(accel0));

const struct device *const rtc_2 = DEVICE_DT_GET(DT_ALIAS(rtc2));
struct counter_top_cfg ctr_top;

int rtc_tick = 0;
int accel_tick = 0;
int ble_tick = 0;

struct sensor_blk {
    //uint8_t hour;
    //uint8_t minute;
    //uint8_t second;
    //float lat;
    //char ns;
    //float lon;
    //char ew;
    //float alt;
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


void rtc_interrupt(const struct device *dev, void *user_data) {
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);
    rtc_tick = 1;
    accel_tick = 0;
    ble_tick = 0;
}

void init_rtc(void) {
    ctr_top.ticks = counter_us_to_ticks(rtc_2, UPDATE);
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

void read_gnss(struct time *neotime, 
    float* lat, char *ns, float *lon, char *ew, float *alt, int *sat) {
    int rc = neo_api->fetch_data(neo_dev);
    if (rc) {
        LOG_WRN("error fetching gnss data\n");
        return;
    }
    neo_api->get_time(neo_dev, neotime);
    neo_api->get_latitude(neo_dev, lat);
    neo_api->get_ns(neo_dev, ns);
    neo_api->get_longitude(neo_dev, lon);
    neo_api->get_ew(neo_dev, ew);
    neo_api->get_altitude(neo_dev, alt);
    neo_api->get_satellites(neo_dev, sat);
}


int read_sensors(double *temp, double *hum, double *press, 
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

    *x_accel = sensor_value_to_double(&x_raw);
    *y_accel = sensor_value_to_double(&y_raw);
    *z_accel = sensor_value_to_double(&z_raw);
    *hum = sensor_value_to_double(&hum_raw);
    *temp = sensor_value_to_double(&temp_raw);
    *press = sensor_value_to_double(&pressure_raw);
    *gas = sensor_value_to_double(&gas_raw);
 
    return 0;
}

// TODO: trigger from ble connect 
void read_loop(const struct device *flash_dev, uint8_t write_block_size) {
    // Triggered from bluetooth wakeup
    uint8_t size = 0;
    flash_read(flash_dev, TEST_PARTITION_OFFSET, &size, write_block_size);
    uint32_t offset = 2 * FLASH_PAGE_SIZE + TEST_PARTITION_OFFSET;
    struct sensor_blk sensors;
    // Unpacks data since last bluetooth grab
    // Im blue dabadeebadubah
    sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, LED_OFF);
    sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, LED_MAX);
    sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, LED_OFF);
    LOG_INF("read size: %d\n", size);
    for (int i = 0; i < size; i++) {

        // read out single struct of data
        flash_read(flash_dev, offset, &sensors, sizeof(sensors));
        offset += sizeof(sensors);
        LOG_INF("read: %f     %f      %f       %f\n", sensors.temp, sensors.hum, sensors.press, sensors.gas);
        // TODO: send sensors struct over bluetooth
    }
    sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, LED_OFF);
    sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, LED_OFF);
    sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, LED_OFF);

}

void write_loop(const struct device *flash_dev, int flag, uint8_t write_block_size) {
    struct time neotime;
    struct sensor_blk sensors;
    int sat;
    int err;
    uint32_t size = 0;
    flash_read(flash_dev, TEST_PARTITION_OFFSET, &size, sizeof(uint32_t));
    LOG_INF("init size: %d\n", size);
    uint32_t offset =  2 * FLASH_PAGE_SIZE + TEST_PARTITION_OFFSET + ((size) * (sizeof(sensors)));
    // Im red like roses
    sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, LED_MAX);
    sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, LED_OFF);
    sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, LED_OFF);
    LOG_INF("Starting write loop.\n");
    for (int i = 0; i < MAX_SAMPLES; i++) {
        // Sleep for some time before grabbing more data
        k_msleep(SAMPLE_RATE);
        //read_gnss(&neotime, &(sensors.lat), &(sensors.ns), &(sensors.lon), &(sensors.ew), &(sensors.alt), &sat);
        uint8_t byte_array[sizeof(struct sensor_blk)];
        err = read_sensors(&(sensors.temp), &(sensors.hum), &(sensors.press), &(sensors.gas), &(sensors.x_accel), &(sensors.y_accel), &(sensors.z_accel));
        if (err) {
            LOG_WRN("Sensor readings invalid.\n");
            continue;
        }
        //sensors.hour = neotime.hour;
        //sensors.minute = neotime.min;
        //sensors.second = neotime.sec;
        if (flag) {
            sensors.x_accel = 255;
            sensors.y_accel = 255;
            sensors.z_accel = 255;
        }
        LOG_INF("temp: %f\n", sensors.temp);
        memcpy(&byte_array, &(sensors), sizeof(sensors));
        uint8_t write_cycles = sizeof(sensors) / write_block_size;
        struct sensor_blk sensor_dum;
        uint8_t byte_arr2[sizeof(struct sensor_blk)];
        LOG_INF("og: %f     %f      %f       %f        %f       %f        %f\n", 
            sensors.temp, sensors.hum, sensors.press, sensors.gas, sensors.x_accel, sensors.y_accel, sensors.z_accel);
        LOG_INF("Write cycles: %u", write_cycles);
        for (int j = 0; j < write_cycles; j++) {
            offset = 2 * FLASH_PAGE_SIZE + TEST_PARTITION_OFFSET + ((size) * (sizeof(sensors))) + j * write_block_size;
            flash_write(flash_dev, offset, &byte_array[j * write_block_size], write_block_size);
        }
        size++;
    }

    flash_erase(flash_dev, TEST_PARTITION_OFFSET, FLASH_PAGE_SIZE);
    if (flash_write(flash_dev, TEST_PARTITION_OFFSET, &size, write_block_size)) {
         LOG_WRN("Size not updated");   
    }
    sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, LED_OFF);
    sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, LED_OFF);
    sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, LED_OFF);
    return;
}

void led_flash(int32_t time) {
    sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, LED_MAX);
    sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, LED_MAX);
    sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, LED_MAX);
    k_msleep(time);
    sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, LED_OFF);
    sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, LED_OFF);
    sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, LED_OFF);
}


static void accel_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	// Reached when threshold reach 
    LOG_WRN("Rapid acceleration detected!\n");
    accel_tick = 1;
    rtc_tick = 0;
    ble_tick = 0;
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
    //err |= init_gnss();
    if (err) {
        return err;
    }
    int flag;
    uint32_t size = 0;
    flash_erase(flash_dev, TEST_PARTITION_OFFSET, FLASH_PAGE_SIZE * 4);
    flash_write(flash_dev, TEST_PARTITION_OFFSET, &size, write_block_size);
    rtc_tick = 1;
    int k = 0;
    while(1) {  
        // Loop occurs on every wakeup from idle or after every occurance
        if (rtc_tick) {
            // Woke from rtc
            LOG_INF("Woke from rtc.\n");
            flag = 0;
            write_loop(flash_dev, flag, write_block_size);
            rtc_tick = 0;
            //init_accel_trig(accel, true);
            read_loop(flash_dev, write_block_size);
        } else if (accel_tick) {
            // Woke from accel
            flag = 1;
            write_loop(flash_dev, flag, write_block_size);
            //init_accel_trig(accel ,true);
            accel_tick = 0;
        } else if (ble_tick) {
            // TODO: Woke from BLE
        }
        LOG_INF("Napping... zzz...\n");
        // epilepsy check begin shutdown process
        int32_t time_flashing = 1000; 
        for (int j = 1; j < 4; j++) {
            led_flash(time_flashing / j);
        }
        
        while (!rtc_tick && !accel_tick && !ble_tick) {
            k_cpu_idle(); // truly idle CPU while waiting
        }
    }
}