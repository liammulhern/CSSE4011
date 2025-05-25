/**
 * @file   wifi.c
 * @brief  M5Core2 Wi-Fi AP + JSON over TCP
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>


#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/http_client.h>

#include <string.h>
#include <stdio.h>

#include <gui.h>
#include <parser.h>

LOG_MODULE_REGISTER(wifi, LOG_LEVEL_INF);


/* Simple blocking GET into a buffer */
static int fetch_to_buffer(const char *url, uint8_t *buf, size_t buf_sz, size_t *out_len) {
    struct http_request req = {
        .method = HTTP_GET,
        .url = url,
        .recv_buf = buf,
        .recv_buf_len = buf_sz
    };

    int ret = http_client_req(&req, K_SECONDS(5), NULL, NULL);

    if (ret < 0) return ret;

    *out_len = req.content_length_hint;

    return 0;
}
