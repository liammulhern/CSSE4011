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
#include <zephyr/net/net_ip.h>

#include <string.h>
#include <stdio.h>

#include <gui.h>
#include <parser.h>

LOG_MODULE_REGISTER(wifi, LOG_LEVEL_INF);

/* —— Wi-Fi AP parameters —— */
#define AP_SSID       "Zephyr-AP"
#define AP_PSK        "zephyr123"
#define AP_CHANNEL    6

/*—— Network Params ——*/
#define AP_IPV4_ADDR  "192.168.4.1"
#define AP_NETMASK    "255.255.255.0"

/* —— TCP parameters —— */
#define SERVER_PORT   5000
#define MAX_CONN      1
#define IN_BUF_SIZE   128

static int  server_fd = -1;
static int  client_fd = -1;
static char in_buf[IN_BUF_SIZE];

/**
 * @brief  Bring up Wi-Fi in Soft-AP mode.
 */
static void wifi_ap_start(void)
{
    struct net_if *iface = net_if_get_default();
    struct wifi_connect_req_params ap_params = {
        .ssid            = AP_SSID,
        .ssid_length     = strlen(AP_SSID),
        .psk             = AP_PSK,
        .psk_length      = strlen(AP_PSK),
        .channel         = AP_CHANNEL,
        .security        = WIFI_SECURITY_TYPE_PSK,
    };

    int err = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE,
                       iface, &ap_params, sizeof(ap_params));
    if (err) {
        LOG_ERR("Failed to start AP (err %d)", err);
    } else {
        LOG_INF("Started AP SSID=\"%s\" CH=%d",
                AP_SSID, AP_CHANNEL);
    }
}

#define IN_BUF_SIZE  128

/**
 * @brief  Parse a JSON payload of the form
 *         {"type":0,"message":{"x_pos":<f>,"y_pos":<f>}}
 *
 * @param buf  Pointer to raw data (may not be NUL-terminated)
 * @param len  Length of the data in buf
 */
static void parse_incoming(const char *buf, size_t len)
{
    char tmp[IN_BUF_SIZE];
    int  msg_type;
    double x1, y1, x2, y2;

    /* Truncate if too long, copy & NUL terminate */
    if (len >= sizeof(tmp)) {
        len = sizeof(tmp) - 1;
    }

    memcpy(tmp, buf, len);
    tmp[len] = '\0';

    /* Now it's safe to log and sscanf() */
    LOG_INF("Received: %s", tmp);

    /* Extract fields one by one */
    bool ok = extract_int(tmp,  "\"type\":", &msg_type)
            && extract_double(tmp, "\"x1_pos\":", &x1)
            && extract_double(tmp, "\"y1_pos\":", &y1)
            && extract_double(tmp, "\"x2_pos\":", &x2)
            && extract_double(tmp, "\"y2_pos\":", &y2);

    if (ok) {
        gui_draw_localisation_position(x1, y1);
        gui_draw_localisation_position(x2, y2);
    } else {
        LOG_WRN("JSON parse failed");
    }
}

/**
 * @brief  TCP server thread entry.
 *
 *         Binds to any-address:SERVER_PORT, accepts one client at a time,
 *         reads JSON, parses, and echoes back parsed positions.
 */
void tcp_server_thread(void)
{
    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(SERVER_PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };

    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0) {
        LOG_ERR("socket() failed: %d", errno);
        return;
    }

    if (bind(server_fd, (struct sockaddr *)&addr,
             sizeof(addr)) < 0) {
        LOG_ERR("bind() failed: %d", errno);
        close(server_fd);
        return;
    }

    if (listen(server_fd, MAX_CONN) < 0) {
        LOG_ERR("listen() failed: %d", errno);
        close(server_fd);
        return;
    }

    LOG_INF("TCP server listening on port %d", SERVER_PORT);

    for (;;) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            LOG_ERR("accept() failed: %d", errno);
            continue;
        }
        LOG_INF("Client connected");

        /* Read until client disconnects */
        while (1) {
            int recv_len = recv(client_fd,
                                in_buf, IN_BUF_SIZE - 1, 0);
            if (recv_len <= 0) {
                LOG_INF("Client disconnected");
                close(client_fd);
                client_fd = -1;
                break;
            }
            in_buf[recv_len] = '\0';
            parse_incoming(in_buf, recv_len);
        }
    }
}

/**
 * @brief  Configure a static IPv4 address on the AP interface.
 */
static void assign_static_ip(void)
{
    struct net_if *iface = net_if_get_default();
    struct in_addr ipaddr, gw, netmask;

    net_addr_pton(AF_INET, AP_IPV4_ADDR, &ipaddr);
    net_addr_pton(AF_INET, AP_IPV4_ADDR, &gw);
    net_addr_pton(AF_INET, AP_NETMASK,   &netmask);

    net_if_ipv4_addr_add(iface, &ipaddr, NET_ADDR_MANUAL, 0);
    net_if_ipv4_set_gw(iface, &gw);

    LOG_INF("Assigned static IP %s/%s",
            AP_IPV4_ADDR, AP_NETMASK);
}

K_THREAD_DEFINE(tcp_thread, 8192, tcp_server_thread,
                NULL, NULL, NULL,
                7, 0, 0);

/**
 * @brief  Application entry point.
 */
void wifi_init(void)
{
    LOG_INF("M5Core2 GUI + Wi-Fi AP example");

    /* 2) Start Wi-Fi Soft-AP */
    wifi_ap_start();

    /* 3) Assign a static IPv4 address so TCP can bind */
    assign_static_ip();
}
