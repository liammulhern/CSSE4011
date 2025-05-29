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

#include <flash.h>
#include <bluetooth.h>
#include <sensors.h>
#include <led.h>
#include <rtc.h>

#define SAMPLE_RATE 500

LOG_MODULE_REGISTER(tracker, LOG_LEVEL_INF);


void read_loop(const struct device *flash_dev, uint8_t write_block_size) {

    struct flash_consts flash_vars;
    flash_read(flash_dev, TEST_PARTITION_OFFSET, &flash_vars, sizeof(struct flash_consts));
    struct sensor_blk sensors;
    uint32_t offset = flash_vars.tail + (flash_vars.read_size * sizeof(struct sensor_blk));
    for (int i = flash_vars.read_size; i < flash_vars.size; i++) {
        if (offset >= 0x00080000) {
            offset = TEST_PARTITION_OFFSET + FLASH_PAGE_SIZE;
        }
        if (offset == flash_vars.head) {
            break;
        }
        flash_read(flash_dev, offset, &sensors, sizeof(sensors));
        offset += sizeof(sensors);
        flash_vars.read_size++;
        LOG_INF("Temp: %u, Hum: %u, Press: %u, Gas: %u, x: %d, y: %d, z %d", sensors.temp, sensors.hum, sensors.press, sensors.gas, sensors.x_accel, sensors.y_accel, sensors.z_accel);
        LOG_INF("Read: time: %u   uptime: %d   lat: %f   lon: %f   alt: %f", sensors.time, sensors.uptime, (double)sensors.lat, (double)sensors.lon, (double)sensors.alt);
        pack_sensor_data(&sensors);
    }
    write_consts(flash_dev, write_block_size, flash_vars.size, flash_vars.read_size, flash_vars.head, flash_vars.tail, flash_vars.wrap_around);
    return;
}



void write_loop(const struct device *flash_dev, uint8_t write_block_size) {
    struct sensor_blk sensors;
    int sat;
    int err;
    uint8_t gnss_retry = 0;
    uint8_t sensor_retry = 0;

sensor_goto:
    k_msleep(200);
    err = read_sensors(&(sensors.temp), &(sensors.hum), &(sensors.press), &(sensors.gas), &(sensors.x_accel), &(sensors.y_accel), &(sensors.z_accel));
    if (err) {
        sensor_retry++;
        if (sensor_retry > 5) {
            k_msleep(SAMPLE_RATE);
            LOG_WRN("Too many incorrect attempts at reading sensors.\n");
            sensors.temp = 0;
            sensors.hum = 0;
            sensors.press = 0;
            sensors.gas = 0;
            sensors.x_accel = 0;
            sensors.y_accel = 0;
            sensors.z_accel = 0;
        } else {
            goto sensor_goto;
        }
    }
    err = init_gnss();
gnss_goto:
    k_msleep(1500);
    err |= read_gnss(&(sensors.time), &(sensors.lat), &(sensors.ns), &(sensors.lon), &(sensors.ew), &(sensors.alt), &sat);
    if (err) {
        gnss_retry++;
        if (gnss_retry > 5) {
            // Attempted to gather data several times and was unsuccesful
            LOG_WRN("Too many incorrect attempts at reading gnss.\n");
            sensors.lat = 0;
            sensors.lon = 0;
            sensors.alt = 0;
            sensors.time = 0;
        } else {
            goto gnss_goto;
        }
    }
    
    flash_write_sensor(flash_dev, write_block_size, sensors);
    LOG_INF("Write: time: %u   lat: %f   lon: %f   alt: %f", sensors.time, (double)sensors.lat, (double)sensors.lon, (double)sensors.alt);
}

int init_all() {
    int err = 0;

    //const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

    /* BLUETOOTH ADDITIONS*/
    err = init_bluetooth();
    if (err) {
        printk("[TRACKER] Bluetooth init failed\n");
        return -EINVAL;
    }
    /*--------------------*/

    init_rtc();
    init_led();
    bind_sensors();
    err = init_sensors();
    //err |= init_gnss();
    if (err) {
        LOG_WRN("error setting up devices. Shutting down.\n");
        return err;
    }

   err = init_anymotion();
    if (err) {
        LOG_WRN("Failed to setup acceleration trigger!\n");
    }

    set_rtc_tick(1);
    return 0;
}


int main(void) {

    const struct device *const cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    const struct device *flash_dev = TEST_PARTITION_DEVICE;
    struct flash_parameters flash_params;
    memcpy(&flash_params, flash_get_parameters(flash_dev), sizeof(flash_params));
    uint8_t write_block_size = flash_params.write_block_size;
     if (!device_is_ready(flash_dev)) {
		LOG_WRN("Internal storage device not ready\n");
        return EINVAL;
	}
    if (!device_is_ready(cons)) {
		LOG_WRN("%s: device not ready.\n", cons->name);
		return EINVAL;
	}
    k_msleep(2000);
    int err = init_all();
    if (err) {
        LOG_WRN("Error in init, shutting down");
    }
    write_init_consts(flash_dev, write_block_size);
    k_msleep(2000);
    while(1) {  
        // Loop occurs on every wakeup from idle or after every occurance
        if (get_rtc_tick()) {
            // Woke from rtc
            LOG_INF("Woke from rtc");
            red_led();
            write_loop(flash_dev, write_block_size);
            set_rtc_tick(0);
            black_led();
            // TODO: LAST THING TO DO!!! check GPS coords is close to a base node.

            // TODO: Start advertising and set ble_tick

            /* BLUETOOTH ADDITIONS*/
             // Advertising started, ble_tick set on connection (in bluetooth.c file); accessible through get_ble_tick()
            start_advertising();
            //internally sleeps in stop advertising function
            k_msleep(3000);
            //Check that conncetion isn't established
            if (!get_ble_tick()){
                stop_advertising();
            }
            /*--------------------*/
            //read_loop(flash_dev, write_block_size);
            

        } else if (get_accel_tick()) {
            // Woke from accel
            LOG_WRN("Rapid motion detected!");
            green_led();
            write_loop(flash_dev, write_block_size);
            set_accel_tick(0);
            black_led();
        } 
        if (get_ble_tick()) {
            // TODO: Woke from BLE
            LOG_WRN("The bluetooth device has connected!");
            blue_led();
            read_loop(flash_dev, write_block_size);
            black_led();
            // TODO: Stop advertising and disconnect. 

            /* BLUETOOTH ADDITIONS*/
             // Advertising started, ble_tick set on connection (in bluetooth.c file); accessible through get_ble_tick()
            stop_advertising_and_disconnect();
            /*--------------------*/

        }
        LOG_INF("Napping... zzz...\n");
        white_led_flash();
        while (!get_rtc_tick() && !get_accel_tick() && !get_ble_tick()) {
            k_cpu_idle(); // truly idle CPU while waiting
        }
    }
}