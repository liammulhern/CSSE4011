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
    gui_init();
}

K_THREAD_DEFINE(gui_tid, GUI_THREAD_STACK_SIZE, gui_thread, NULL, NULL, NULL, GUI_THREAD_PRIORITY, 0, 0);
K_THREAD_DEFINE(wifi_tid, 8192, wifi_thread, NULL, NULL, NULL, GUI_THREAD_PRIORITY, 0, 0);
