#include <zephyr/kernel.h>
#include <zephyr/data/json.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include <parser.h>

LOG_MODULE_REGISTER(parser_module, LOG_LEVEL_DBG);

/* Global array to hold parsed notifications */
notification_t notifications[MAX_NOTIFICATIONS];
size_t notification_count = 0;

/* 1) Descriptor for a single notification object */
static const struct json_obj_descr notif_descr[] = {
    JSON_OBJ_DESCR_PRIM_NAMED(notification_t, "id", id, JSON_TOK_STRING_BUF),
    JSON_OBJ_DESCR_PRIM_NAMED(notification_t, "type", type, JSON_TOK_STRING_BUF),
    JSON_OBJ_DESCR_PRIM_NAMED(notification_t, "message", message, JSON_TOK_STRING_BUF),
    JSON_OBJ_DESCR_PRIM_NAMED(notification_t, "timestamp", timestamp, JSON_TOK_STRING_BUF),
};

/* 2) Wrapper struct for the root‐level array */
struct notifications_root {
    notification_t notifications[MAX_NOTIFICATIONS];
    size_t notifications_len;
};

/* 3) Descriptor for the array of objects */
static const struct json_obj_descr arr_descr[] = {
    JSON_OBJ_DESCR_OBJ_ARRAY(struct notifications_root,
                             notifications,
                             MAX_NOTIFICATIONS,
                             notifications_len,
                             notif_descr, ARRAY_SIZE(notif_descr)),
};

/**
 * Parse a body_buf containing a JSON _array_ of notifications:
 *
 *   [
 *     {"id":"1", "type":"info", ...},
 *     {"id":"2", "type":"warning", ...}
 *   ]
 */void parse_notifications(char *json_buf, size_t buf_len)
{
    struct notifications_root wrap = {0};
    int rc;

    rc = json_arr_parse(json_buf, buf_len, arr_descr, &wrap); 

    if (rc < 0) {
        LOG_ERR("JSON parse failed: %d\n", rc);
        return;
    }

    /* Copy out and parse timestamps */
    notification_count = wrap.notifications_len;

    for (size_t i = 0; i < notification_count; i++) {
        LOG_DBG("Notification %zu: id=%s, type=%s, message=%s, timestamp=%s",
               i, wrap.notifications[i].id,
               wrap.notifications[i].type,
               wrap.notifications[i].message,
               wrap.notifications[i].timestamp);

        /* copy raw fields */
        memcpy(&notifications[i], &wrap.notifications[i], sizeof(notification_t));
    }
}
