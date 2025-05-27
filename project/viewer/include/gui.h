#ifndef GUI_H_
#define GUI_H_

#define GUI_THREAD_STACK_SIZE 4096
#define GUI_THREAD_PRIORITY 5

#include <parser.h>
#include <lvgl.h>

#define CONFIG_GUI

int gui_init();

void gui_thread(void);

void gui_update_wifi_status(const char *symbol, const char *status, const char *activity);

void gui_show_notifications_screen();

#endif // GUI_H_
