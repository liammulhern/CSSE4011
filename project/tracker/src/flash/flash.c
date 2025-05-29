#include <flash.h>

void write_init_consts(const struct device *flash_dev, uint8_t write_block_size) {
    write_consts(flash_dev, write_block_size, 0, 0, TEST_PARTITION_OFFSET + FLASH_PAGE_SIZE, TEST_PARTITION_OFFSET + FLASH_PAGE_SIZE, 0);
}

void write_consts(const struct device *flash_dev, uint8_t write_block_size, 
    uint32_t size, uint32_t read_size, uint32_t head, uint32_t tail, uint8_t wrap_around) {
    //LOG_INF("Writing constants: %u    %u     %u      %u       %u\n", size, read_size, head, tail, wrap_around);
    struct flash_consts flash_vars;
    flash_vars.size = size;
    flash_vars.read_size = read_size;
    flash_vars.head = head;
    flash_vars.tail = tail;
    flash_vars.wrap_around = wrap_around;
    uint8_t flash_byte_arr[sizeof(struct flash_consts)];
    memcpy(&flash_byte_arr, &flash_vars, sizeof(struct flash_consts));
    uint8_t write_cycles = sizeof(struct flash_consts) / write_block_size;
    uint32_t offset;
    flash_erase(flash_dev, TEST_PARTITION_OFFSET, FLASH_PAGE_SIZE);
    for (int j = 0; j < write_cycles; j++) {
        offset = TEST_PARTITION_OFFSET + j * write_block_size;
        flash_write(flash_dev, offset, &flash_byte_arr[j * write_block_size], write_block_size);
    }
}

void flash_write_sensor(const struct device *flash_dev, uint8_t write_block_size, struct sensor_blk sensors) {
    
    uint8_t byte_array[sizeof(struct sensor_blk)];
    uint8_t write_cycles = sizeof(sensors) / write_block_size;
    struct flash_consts flash_vars;
    flash_read(flash_dev, TEST_PARTITION_OFFSET, &flash_vars, sizeof(struct flash_consts));
    uint32_t offset = FLASH_PAGE_SIZE + TEST_PARTITION_OFFSET + ((flash_vars.size) * (sizeof(sensors)));

     if (offset >= 0x00080000) {
        offset = FLASH_PAGE_SIZE + TEST_PARTITION_OFFSET;
        // Wrap around to start of writtable flash erase oldest 64 data structs. 
        flash_erase(flash_dev, offset, FLASH_PAGE_SIZE);
        flash_vars.wrap_around++;
        if (flash_vars.size <= 64) {
            flash_vars.size = 0;
        } else {
            flash_vars.size -= 64; 
        }
        if (flash_vars.read_size <= 64) {
            flash_vars.read_size = 0;
        } else {
            flash_vars.read_size -= 64;
        }
        flash_vars.head = offset;
        flash_vars.tail = flash_vars.head + FLASH_PAGE_SIZE; // 1 Page beyond head where to start reading from.
    } else if ((offset - TEST_PARTITION_OFFSET) % FLASH_PAGE_SIZE == 0) {
        flash_erase(flash_dev, offset, FLASH_PAGE_SIZE);
        if (flash_vars.wrap_around) {
            flash_vars.head = offset;
            flash_vars.tail = flash_vars.head + FLASH_PAGE_SIZE;
            if (flash_vars.size <= 64) {
                flash_vars.size = 0;
            } else {
                flash_vars.size -= 64; 
            }
            if (flash_vars.read_size <= 64) {
                flash_vars.read_size = 0;
            } else {
                flash_vars.read_size -= 64;
            }

        }
    }

    sensors.uptime = k_uptime_seconds();
    //sensors.time += sensors.uptime;
    memcpy(&byte_array, &(sensors), sizeof(sensors));
    for (int j = 0; j < write_cycles; j++) {
        offset = FLASH_PAGE_SIZE + TEST_PARTITION_OFFSET + ((flash_vars.size) * (sizeof(sensors))) + j * write_block_size;
        flash_write(flash_dev, offset, &byte_array[j * write_block_size], write_block_size);
    }
    flash_vars.size++;
    flash_vars.head = offset;
    write_consts(flash_dev, write_block_size, flash_vars.size, flash_vars.read_size, flash_vars.head, flash_vars.tail, flash_vars.wrap_around);
}
