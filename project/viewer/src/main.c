/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <gui.h>
#include <parser.h>
#include <wifi.h>


#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main_module, LOG_LEVEL_INF);

/* Application entry point */
int main(void)
{

    static notification_t dummy[MAX_NOTIFICATIONS] = {
        {
            .id        = "1",
            .type      = "info",
            .message   = "System startup complete.",
            .timestamp = "2025-05-27T12:00:00Z",
        },
        {
            .id        = "2",
            .type      = "warning",
            .message   = "Low battery detected!",
            .timestamp = "2025-05-27T12:05:00Z",
        },
        {
            .id        = "3",
            .type      = "alert",
            .message   = "Over-temperature alarm!",
            .timestamp = "2025-05-27T12:10:00Z",
        },
    };

    /* 1) Parse the notifications from a JSON string */
    notifications[0] = dummy[0];
    notifications[1] = dummy[1];
    notifications[2] = dummy[2];

    /* 3) Tell the GUI how many to draw */
    notification_count = 3;

    #ifdef CONFIG_GUI
    gui_init();

    /* 4) Show them on screen */
    gui_show_notifications_screen();
    #endif
}

#ifdef CONFIG_GUI
K_THREAD_DEFINE(gui_tid, GUI_THREAD_STACK_SIZE, gui_thread, NULL, NULL, NULL, GUI_THREAD_PRIORITY, 0, 0);
#endif

K_THREAD_DEFINE(wifi_tid, 6000, wifi_thread, NULL, NULL, NULL, GUI_THREAD_PRIORITY, 0, 0);
