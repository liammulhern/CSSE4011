#ifndef TRACKER_LED_H
#define TRACKER_LED_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_sx1509b.h>

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

void init_led(void);

void set_led(int red, int blue, int green);

void black_led(void);

void red_led(void);

void green_led(void);

void blue_led(void);

void white_led(void);

void white_led_flash(void);

#endif
