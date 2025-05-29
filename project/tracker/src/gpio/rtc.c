#include <rtc.h>

const struct device *const rtc_2 = DEVICE_DT_GET(DT_ALIAS(rtc2));
struct counter_top_cfg ctr_top;

uint8_t rtc_tick = 0;

uint8_t get_rtc_tick(void) {
    return rtc_tick;
}

void set_rtc_tick(int set) {
    rtc_tick = set;
}
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
