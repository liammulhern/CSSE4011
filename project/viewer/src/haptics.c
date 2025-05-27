
// src/vibrator.c

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(vibrator, LOG_LEVEL_INF);

/* 1) Grab the vib_motor regulator from Devicetree */
#define VIB_NODE DT_NODELABEL(vib_motor)
#define VIB_DEV DEVICE_DT_GET(VIB_NODE)

/* 2) Semaphore to signal the worker thread */
static K_SEM_DEFINE(vib_sem, 0, 1);

/* 3) Parameters to pass to the thread */
static uint32_t vib_count;
static uint32_t vib_ms;

/**
 * Public API: ask the background thread to buzz 'count' times,
 * each pulse lasting 'ms' milliseconds (with 'ms' ms off between).
 */
void haptic_motor_vibrate(uint32_t count, uint32_t ms)
{
    vib_count = count;
    vib_ms = ms;

    /* wake up the worker */
    k_sem_give(&vib_sem);
}

/* Worker thread: waits for requests and runs the pattern */
void haptics_thread(void)
{
    if (!device_is_ready(VIB_DEV)) {
        LOG_ERR("Vibrator regulator not ready");
        return;
    }

    while (1) {
        /* wait until someone calls motor_vibrate_times() */
        k_sem_take(&vib_sem, K_FOREVER);

        for (uint32_t i = 0; i < vib_count; i++) {
            /* turn on */
            if (regulator_enable(VIB_DEV) < 0) {
                LOG_ERR("Failed to enable vibrator");
                break;
            }

            k_msleep(vib_ms);

            /* turn off */
            if (regulator_disable(VIB_DEV) < 0) {
                LOG_ERR("Failed to disable vibrator");
                break;
            }

            /* gap between pulses */
            k_msleep(vib_ms);
        }
    }
}

/* 4) Spawn the worker at boot */

