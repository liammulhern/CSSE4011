#ifndef PARSER_H_
#define PARSER_H_

#include <zephyr/kernel.h>

#define MAX_NOTIFICATIONS    10
#define MAX_ID_LEN           36
#define MAX_TYPE_LEN         16
#define MAX_MESSAGE_LEN      128
#define MAX_TIMESTAMP_LEN    21   /* "YYYY-MM-DDTHH:MM:SSZ" + '\0' */

/* 1) In-memory notification struct */
typedef struct {
    char id[MAX_ID_LEN];
    char type[MAX_TYPE_LEN];
    char message[MAX_MESSAGE_LEN];
    char timestamp[MAX_TIMESTAMP_LEN];  /* raw ISO-8601 */
} notification_t;


extern notification_t notifications[MAX_NOTIFICATIONS];
extern size_t notification_count;

void parse_notifications(char *json_buf, size_t buf_len);

#endif // PARSER_H_
