#ifndef TRACKER_RTC_H
#define TRACKER_RTC_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>


#define ALARM_CHANNEL_ID 0
#define UPDATE 60

uint8_t get_rtc_tick(void);

void set_rtc_tick(int set);

void rtc_interrupt(const struct device *dev, void *user_data);

void init_rtc(void);

#endif