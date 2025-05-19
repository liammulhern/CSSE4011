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
#include <zephyr/drivers/gpio/gpio_sx1509b.h>
#include <drivers/gnss/ublox_neo_m9n.h>
#include <zephyr/pm/pm.h>


#define MAX_SAMPLES 10
#define TEST_PARTITION	storage_partition
#define TEST_PARTITION_OFFSET FIXED_PARTITION_OFFSET(TEST_PARTITION)
#define TEST_PARTITION_DEVICE FIXED_PARTITION_DEVICE(TEST_PARTITION)

#define FLASH_PAGE_SIZE   4096

#define NEO_SERIAL DT_NODELABEL(neom9n)


#define LED_MAX 255
#define LED_OFF 0
#define NUMBER_OF_LEDS 3
#define GREEN_LED DT_GPIO_PIN(DT_NODELABEL(led0), gpios)
#define BLUE_LED DT_GPIO_PIN(DT_NODELABEL(led1), gpios)
#define RED_LED DT_GPIO_PIN(DT_NODELABEL(led2), gpios)

static const gpio_pin_t rgb_pins[] = {
	RED_LED,
	GREEN_LED,
	BLUE_LED,
};

const struct device *sx1509b_dev;

#define ALARM_CHANNEL_ID 0
#define UPDATE 20000// Update window (interrupts)
#define SAMPLE_RATE 1000 // Time between samples grabbed in a row while awake
LOG_MODULE_REGISTER(tracker, LOG_LEVEL_INF);

struct counter_top_cfg ctr_top;

const struct device *const rtc_2 = DEVICE_DT_GET(DT_ALIAS(rtc2));


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

// TODO: set rtc_wakeup to 0 on ble interupt, ble wakeup to 1 on ble interrupt
// TODO: send data in read_loop over bluetooth. 
static int rtc_wakeup;
static int ble_wakeup;

void rtc_interrupt(const struct device *dev, void *user_data) {
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);
    rtc_wakeup = 1;
    ble_wakeup = 0;
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
}


void write_to_flash(const struct device *flash_dev, struct sensor_blk sensors, uint32_t size, off_t offset) { 
    flash_write(flash_dev, offset, &sensors, sizeof(sensors));
}


int read_loop(const struct device *flash_dev) {
    // Triggered from bluetooth wakeup
    uint32_t size = 0;
    uint32_t offset = 0 + sizeof(int);
    flash_read(flash_dev, 0, &size, sizeof(int));
    struct sensor_blk sensors;
    // Unpacks data since last bluetooth grab
    // Im blue dabadeebadubah
    sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, LED_OFF);
    sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, LED_MAX);
    sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, LED_OFF);
    for (int i = 0; i < size; i++) {

        // read out single struct of data
        flash_read(flash_dev, offset, &sensors, sizeof(sensors));
        offset += sizeof(sensors);
        LOG_DBG("Sensor data read...");
        // TODO: send sensors struct over bluetooth
    }
    sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, LED_OFF);
    sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, LED_OFF);
    sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, LED_OFF);
}


void write_loop(const struct device *flash_dev) {
    struct time neotime;
    int sat;
    struct sensor_blk sensors;
    uint32_t size = 0;
    flash_read(flash_dev, 0, &size, sizeof(int));
    uint32_t offset = sizeof(int) + ((size) * (sizeof(sensors)));
    // Im red like roses
    sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, LED_MAX);
    sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, LED_OFF);
    sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, LED_OFF);
    for (int i = 0; i < MAX_SAMPLES; i++) {
        // Sleep for some time before grabbing more data
        k_msleep(SAMPLE_RATE);
        read_gnss(&neotime, &(sensors.lat), &(sensors.ns), &(sensors.lon), &(sensors.ew), &(sensors.alt), &sat);
        read_sensors(&(sensors.temp), &(sensors.hum), &(sensors.press), &(sensors.gas), &(sensors.x_accel), &(sensors.y_accel), &(sensors.z_accel));
        sensors.hour = neotime.hour;
        sensors.minute = neotime.minute;
        sensors.second = neotime.second;
        size++;
        write_to_flash(flash_dev, sensors, size, offset);
        offset += sizeof(sensors);
        LOG_DBG("Sensor data written");
    }
    flash_erase(flash_dev, 0, sizeof(size));
    flash_write(flash_dev, 0, &size, sizeof(size));
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

int main(void) { 
    init_rtc();
    const struct device *flash_dev = TEST_PARTITION_DEVICE;
    struct flash_parameters flash_params;
    memcpy(&flash_params, flash_get_parameters(flash_dev), sizeof(flash_params));
    int err = 0;
    if (!device_is_ready(flash_dev)) {
		printf("Internal storage device not ready\n");
        return EINVAL;
	}

    sx1509b_dev = DEVICE_DT_GET(DT_NODELABEL(sx1509b));
    // Setup led
	if (!device_is_ready(sx1509b_dev)) {
		printk("sx1509b: device not ready.\n");
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
    accel = DEVICE_DT_GET_ONE(st_lis2dh12);
    err = init_sensors();
    err |= init_gnss();
    if (err) {
        return err;
    }
    write_loop(flash_dev);
    while(1) {
        // Loop occurs on every wakeup from idle or after every occurance
        if (rtc_wakeup) {
            write_loop(flash_dev);
            rtc_wakeup = 0;
        } else if (ble_wakeup) {
            read_loop(flash_dev);
            ble_wakeup = 0; 
        } else {
            // epilepsy check
            int32_t time_flashing = 200; 
            LOG_DBG("Idling thingy52... zzz...");
            for (int i = 1; i < 3; i++) {
                led_flash(time_flashing * i);
            }
            pm_state_force(0, &(struct pm_state_info){
                .state = PM_STATE_SUSPEND_TO_IDLE,
                .substate_id = 0
            });
            k_cpu_idle();
        }
    }
}