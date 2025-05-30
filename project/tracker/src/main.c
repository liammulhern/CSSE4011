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

/*
* Read sensor data out of flash and send it over the established bluetooth connection.
*/
void read_loop(const struct device *flash_dev, uint8_t write_block_size) {

    struct flash_consts flash_vars;
    flash_read(flash_dev, TEST_PARTITION_OFFSET, &flash_vars, sizeof(struct flash_consts));
    struct sensor_blk sensors;
    for (int i = flash_vars.read_size; i < flash_vars.size; i++) {
        flash_read_sensor(flash_dev, write_block_size,  &flash_vars, &sensors);
        // LOG_INF("Read: temp: %u   press: %u    hum: %u    gas: %u    x_accel: %d    y_accel: %d    z_accel: %d", 
        //         sensors.temp, sensors.press, sensors.hum, sensors.gas, sensors.x_accel, sensors.y_accel, sensors.z_accel);
        // LOG_INF("Read: time: %u   uptime: %u    lat: %f   lon: %f   alt: %f\n", 
        //         sensors.time, sensors.uptime, (double)sensors.lat, (double)sensors.lon, (double)sensors.alt);
        pack_sensor_data(&sensors);
    }
    write_consts(flash_dev, write_block_size, flash_vars.size, flash_vars.read_size, flash_vars.head, flash_vars.tail, flash_vars.wrap_around);
    return;
}


/*
* Gather the sensor data and pack it into flash, in a contigous loop, erasing oldest packed structs.
*/
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
    k_msleep(1000);
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
    sensors.uptime = k_uptime_seconds();
    flash_write_sensor(flash_dev, write_block_size, sensors);
    //LOG_INF("Write: temp: %u   press: %u    hum: %u    gas: %u    x_accel: %d    y_accel: %d    z_accel: %d", 
    //    sensors.temp, sensors.press, sensors.hum, sensors.gas, sensors.x_accel, sensors.y_accel, sensors.z_accel);
    //LOG_INF("Write: time: %u   uptime: %u    lat: %f   lon: %f   alt: %f", 
    //    sensors.time, sensors.uptime, (double)sensors.lat, (double)sensors.lon, (double)sensors.alt);
}

/*
* Initialise and bind all sensors, timers and GPIO devices.
*/
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

/*
* Main function loop, enters idle mode (low power) while waiting for interrupts.
*/
int main(void) {

    // Bind flash device.
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

    int err = init_all();
    if (err) {
        LOG_WRN("Error in init, shutting down");
    }
    // Verify if the there is data in the flash.
    uint8_t buf = 0xFF; 
    flash_read(flash_dev, TEST_PARTITION_OFFSET, &(buf), sizeof(buf));
    LOG_INF("BUFFER %u", buf);
    if (buf == 0xFF) {
        // Flash consts have not been written yet.
        write_init_consts(flash_dev, write_block_size);
    }

   k_msleep(1500);
    while(1) {  
        // Loop occurs on every wakeup from idle or after every occurance
        if (get_rtc_tick()) {
            // Woke from rtc
            red_led();
            write_loop(flash_dev, write_block_size);
            set_rtc_tick(0);
            black_led();

             // Advertising started, ble_tick set on connection (in bluetooth.c file); accessible through get_ble_tick()
            start_advertising();
            //internally sleeps in stop advertising function
            k_msleep(3000);
            //Check that conncetion isn't established
            if (!get_ble_tick()){
                stop_advertising();
            }
        } else if (accel_tick) {
            // Woke from accel
            LOG_WRN("Rapid motion detected!");
            green_led();
            write_loop(flash_dev, write_block_size);
            accel_tick = 0 ;
            black_led();
        } 
        if (get_ble_tick()) {
            LOG_WRN("The bluetooth device has connected!");
            blue_led();
            read_loop(flash_dev, write_block_size);
            black_led();
             // Advertising started, ble_tick set on connection (in bluetooth.c file); accessible through get_ble_tick()
            stop_advertising_and_disconnect();
            /*--------------------*/

        }
        LOG_INF("Napping... zzz...");
        white_led_flash();
        while (!get_rtc_tick() && !accel_tick && !get_ble_tick()) {
            k_cpu_idle(); // truly idle CPU while waiting
        }
    }
}
