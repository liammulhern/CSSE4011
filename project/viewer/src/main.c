/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <gui.h>
#include <wifi.h>

/* Application entry point */
int main(void)
{
    gui_init();
    wifi_init();
}

K_THREAD_DEFINE(gui_tid, GUI_THREAD_STACK_SIZE, gui_thread, NULL, NULL, NULL, GUI_THREAD_PRIORITY, 0, 0);
