#include <led.h>


const struct device *sx1509b_dev;


void init_led(void) {
     sx1509b_dev = DEVICE_DT_GET(DT_NODELABEL(sx1509b));
    // Setup led
    int err = 0;
	if (!device_is_ready(sx1509b_dev)) {
        return;
	}
    for (int i = 0; i < NUMBER_OF_LEDS; i++) {
		err = sx1509b_led_intensity_pin_configure(sx1509b_dev, rgb_pins[i]);

		if (err) {
			return;
		}
    }
}


void set_led(int red, int blue, int green) {
    sx1509b_led_intensity_pin_set(sx1509b_dev, RED_LED, red);
    sx1509b_led_intensity_pin_set(sx1509b_dev, BLUE_LED, blue);
    sx1509b_led_intensity_pin_set(sx1509b_dev, GREEN_LED, green);
}

void black_led(void) {
    set_led(LED_OFF, LED_OFF, LED_OFF);
}
void red_led(void) {
    set_led(LED_MAX, LED_OFF, LED_OFF);
}

void green_led(void) {
    set_led(LED_OFF, LED_OFF, LED_MAX);
}

void blue_led(void) {
    set_led(LED_OFF, LED_MAX, LED_OFF);
}

void white_led(void) {
    set_led(LED_MAX, LED_MAX, LED_MAX);
}

void white_led_flash(void) {
    for (int i = 0; i < 3; i++) {
        black_led();
        k_msleep(500);
        white_led();
    }
    k_msleep(500);
    black_led();
}