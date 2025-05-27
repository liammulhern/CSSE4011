#include <zephyr/kernel.h>
#include <zephyr/data/json.h>
#include <string.h>
#include <stdio.h>
#include <parser.h>

/* 2) Wrapper for parsing the top-level {"notifications": [...]} */
struct notifications_wrapper {
    notification_t notifications[MAX_NOTIFICATIONS];
    size_t         notifications_len;
};

/* 3a) Descriptor for one notification object */
static const struct json_obj_descr notif_descr[] = {
    JSON_OBJ_DESCR_PRIM_NAMED(notification_t, "id", id, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM_NAMED(notification_t, "type", type, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM_NAMED(notification_t, "message", message, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM_NAMED(notification_t, "timestamp", timestamp, JSON_TOK_STRING),
};

/* 3b) Descriptor for the array named "notifications" */
static const struct json_obj_descr wrapper_descr[] = {
    JSON_OBJ_DESCR_OBJ_ARRAY_NAMED(struct notifications_wrapper,
                                   "notifications",
                                   notifications,
                                   MAX_NOTIFICATIONS,
                                   notifications_len,
                                   notif_descr,
                                   ARRAY_SIZE(notif_descr)),
};

notification_t notifications[MAX_NOTIFICATIONS] = { 0 };
size_t notification_count = 0;

/* 4) Parse + post-process */
void parse_notifications(char *json_buf, size_t buf_len)
{
    struct notifications_wrapper wrap = { 0 };
    int rc = json_obj_parse(json_buf,
                            buf_len,
                            wrapper_descr,
                            ARRAY_SIZE(wrapper_descr),
                            &wrap);

    if (rc < 0) {
        printk("JSON parse failed: %d\n", rc);
        return;
    }

    /* Copy out and parse timestamps */
    notification_count = wrap.notifications_len;
    for (size_t i = 0; i < notification_count; i++) {
        /* copy raw fields */
        memcpy(&notifications[i], &wrap.notifications[i], sizeof(notification_t));
    }
}

