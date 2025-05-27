#ifndef GUI_H_
#define GUI_H_

#include <lvgl.h>
#include <stddef.h>

#define GUI_THREAD_STACK_SIZE 4096
#define GUI_THREAD_PRIORITY 5

#define CARD_MARGIN        8
#define CARD_PADDING       6
#define CARD_RADIUS        5
#define CARD_BORDER_WIDTH  1

#define MAX_SSID_LEN 32
#define MAX_EXTRA_LEN 16

/* Called from any thread to schedule a Wi-Fi status update */
void gui_notify_wifi_status(const char *icon, const char *ssid, const char *extra);

/* Called from any thread to schedule a full notifications-screen redraw */
void gui_notify_notifications_update(void);

int gui_init(void);

void gui_thread(void);

#endif /* GUI_H_ */
