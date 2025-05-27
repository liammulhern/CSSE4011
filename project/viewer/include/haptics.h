#ifndef HAPTICS_H_
#define HAPTICS_H_

#include <zephyr/kernel.h>

void haptic_motor_vibrate(uint32_t count, uint32_t ms);

void haptics_thread(void);

#endif /* HAPTICS_H_ */
