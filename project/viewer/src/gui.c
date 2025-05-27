#include "font/lv_symbol_def.h"
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <parser.h>

#include <display/lv_display.h>

#include <lvgl.h>

LOG_MODULE_REGISTER(gui_module, LOG_LEVEL_INF);

/* Card styling constants */
#define CARD_MARGIN        8
#define CARD_PADDING       6
#define CARD_RADIUS        5
#define CARD_BORDER_WIDTH  1

static lv_obj_t *wifi_lbl;

static void create_notification_card(lv_obj_t *parent,
                                     const notification_t *n)
{
    /* 1) Create the “card” container */
    lv_obj_t *card = lv_obj_create(parent);
    /* clear default style if you want a clean slate */
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, lv_pct(100), LV_SIZE_CONTENT);

    /* 2) Apply style: radius, padding, border, bg, margin */
    lv_obj_set_style_radius(card, CARD_RADIUS, 0);
    lv_obj_set_style_pad_all(card, CARD_PADDING, 0);
    lv_obj_set_style_border_width(card, CARD_BORDER_WIDTH, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_margin_all(card, CARD_MARGIN, 0);

    /* 3) Set up a vertical flex layout */
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card,
                          LV_FLEX_ALIGN_START,  /* main-axis align */
                          LV_FLEX_ALIGN_START,  /* cross-axis align */
                          LV_FLEX_ALIGN_START); /* track align */

    /* 4) Add the “type” label */
    lv_obj_t *type_lbl = lv_label_create(card);

    if (strcmp(n->type, "info") == 0) {
        lv_label_set_text_fmt(type_lbl, "%s [%s]" , LV_SYMBOL_BELL, n->type);
    } else if (strcmp(n->type, "warning") == 0) {
        lv_label_set_text_fmt(type_lbl, "%s [%s]" , LV_SYMBOL_WARNING, n->type);
        lv_obj_set_style_text_color(type_lbl, lv_color_hex(0xFFC107), 0); // yellow
    } else if (strcmp(n->type, "alert") == 0) {
        lv_label_set_text_fmt(type_lbl, "%s [%s]" , LV_SYMBOL_WARNING, n->type);
        lv_obj_set_style_text_color(type_lbl, lv_color_hex(0xDC3545), 0); // red
    } else {
        lv_label_set_text_fmt(type_lbl, "[%s]" , n->type);
        lv_obj_set_style_text_color(type_lbl, lv_color_hex(0x6C757D), 0); // gray
    }

    /* 5) Add the “message” label (wrapping) */
    lv_obj_t *msg_lbl = lv_label_create(card);

    lv_label_set_long_mode(msg_lbl, LV_LABEL_LONG_WRAP);

    lv_obj_set_width(msg_lbl, lv_pct(100));

    lv_label_set_text(msg_lbl, n->message);

    /* 6) Add the “timestamp” */
    lv_obj_t *ts_lbl = lv_label_create(card);
    lv_label_set_text(ts_lbl, n->timestamp);
}

void gui_update_wifi_status(const char *symbol, const char *status, const char *activity)
{
    if (wifi_lbl) {
        lv_label_set_text_fmt(wifi_lbl, "%s Network: %s %s", symbol, status, activity);
    } else {
        LOG_ERR("WiFi label not initialized");
    }
}

void gui_show_notifications_screen()
{
    /* 1) Get the base screen and clear it */
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);

    /* create a network‐status label at the very top */
    wifi_lbl = lv_label_create(scr);
    gui_update_wifi_status(LV_SYMBOL_WIFI, "<disconnected>", "");
    lv_obj_set_width(wifi_lbl, LV_PCT(100));          /* span full width */
    lv_obj_set_style_text_align(wifi_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(wifi_lbl, LV_ALIGN_TOP_MID, 0, 8);    /* y=8px from top */

    /* 2) Create a plain container to hold everything */
    lv_obj_t *scrl = lv_obj_create(scr);

    /* 3) Make it full-screen */
    lv_obj_set_size(scrl, LV_PCT(100), LV_PCT(100));
    lv_obj_set_y(scrl, 32);

    /* 4) Enable vertical scrolling + auto-hiding scrollbar */
    lv_obj_set_scroll_dir(scrl, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(scrl, LV_SCROLLBAR_MODE_AUTO);

    /* 5) Optional: remove its default padding/background if you want “bare” */
    lv_obj_set_style_bg_opa(scrl, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(scrl, 0, 0);

    /* 6) Turn on flex layout so cards stack top→down */
    lv_obj_set_layout(scrl, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scrl, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scrl,
                          LV_FLEX_ALIGN_START, /* main axis */
                          LV_FLEX_ALIGN_START, /* cross axis */
                          LV_FLEX_ALIGN_START);/* track align */

    /* 7) Add one “card” per notification */
    for (size_t i = 0; i < notification_count; i++) {
        create_notification_card(scrl, &notifications[i]);
    }
}


int gui_init() {
    const struct device *display_dev;

    display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

    if (!device_is_ready(display_dev)) {
        LOG_ERR("Device not ready, aborting test");
        return -ENODEV;
    }

    lv_timer_handler();
    display_blanking_off(display_dev);

    return 0;
}

void gui_thread(void) {
    while (1) {
        uint32_t sleep_ms = lv_timer_handler();

        k_msleep(MIN(sleep_ms, INT32_MAX));
    }
}
