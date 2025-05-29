#ifndef TRACKER_FLASH_H
#define TRACKER_FLASH_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include <sensors.h>

#define TEST_PARTITION	storage_partition
#define TEST_PARTITION_OFFSET FIXED_PARTITION_OFFSET(TEST_PARTITION)
#define TEST_PARTITION_DEVICE FIXED_PARTITION_DEVICE(TEST_PARTITION)

#define FLASH_PAGE_SIZE   4096

struct flash_consts {
    uint32_t size;
    uint32_t read_size;
    uint32_t tail;
    uint32_t head;
    uint8_t wrap_around;
};


struct sensor_blk {
    uint32_t time;
    uint32_t uptime;
    float lat;
    float lon;
    float alt;
    int16_t temp;
    int16_t hum;
    int16_t press;
    int16_t gas;
    int16_t x_accel; 
    int16_t y_accel;
    int16_t z_accel;
    char ns;
    char ew; 
    char padding[28];
};

void write_init_consts(const struct device *flash_dev, uint8_t write_block_size);

void flash_write_sensor(const struct device *flash_dev, uint8_t write_block_size, struct sensor_blk sensors);

void write_consts(const struct device *flash_dev, uint8_t write_block_size, 
    uint32_t size, uint32_t read_size, uint32_t head, uint32_t tail, uint8_t wrap_around);

#endif
