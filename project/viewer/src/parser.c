#include <zephyr/kernel.h>
#include <zephyr/data/json.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include <parser.h>
#include <haptics.h>

LOG_MODULE_REGISTER(parser_module, LOG_LEVEL_DBG);

/* Global array to hold parsed notifications */
notification_t notifications[MAX_NOTIFICATIONS];
size_t notification_count = 0;

static char prev_ids[MAX_NOTIFICATIONS][MAX_ID_LEN];
static size_t prev_count = 0;

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

    /* Check if we have any new notifications */
    bool changed = false;

    if (wrap.notifications_len != prev_count) {
        changed = true;
    } else {
        for (size_t i = 0; i < wrap.notifications_len; i++) {
            if (strcmp(wrap.notifications[i].id, prev_ids[i]) != 0) {
                changed = true;
                break;
            }
        }
    }

    /* vibrate 2 pulses of 100ms to alert user */
    if (changed) {
        haptic_motor_vibrate(2, 100);
    }

    /* save off this batch of IDs for next time */
    prev_count = wrap.notifications_len;
    for (size_t i = 0; i < prev_count; i++) {
        strncpy(prev_ids[i], wrap.notifications[i].id, sizeof(prev_ids[i]) - 1);
        prev_ids[i][sizeof(prev_ids[i]) - 1] = '\0';
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
