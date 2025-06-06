/*
 * Copyright (c) 2017 ARM Ltd.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <wifi.h>
#include <env.h>

static uint8_t wifi_connection_retry_count = 0;
static char wifi_rety_dots[WIFI_RETRY_COUNT + 1] = {0};

static K_SEM_DEFINE(wifi_connected, 0, 1);
static K_SEM_DEFINE(ipv4_address_obtained, 0, 1);

static bool got_ipv4 = false;

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;
static struct net_mgmt_event_callback l4_cb;

LOG_MODULE_REGISTER(wifi_module, LOG_LEVEL_INF);

void wifi_connect_status(void);

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
    const struct wifi_status *status = (const struct wifi_status *)cb->info;

    if (status->status)
    {
        LOG_ERR("Connection request failed (%d)\n", status->status);
    }
    else
    {
        LOG_INF("Connected\n");
        k_sem_give(&wifi_connected);
    }
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
    const struct wifi_status *status = (const struct wifi_status *)cb->info;

    if (status->status)
    {
        LOG_ERR("Disconnection request (%d)\n", status->status);

        wifi_rety_dots[wifi_connection_retry_count] = '.';
        wifi_connection_retry_count++;

        if (wifi_connection_retry_count < WIFI_RETRY_COUNT)
        {

            k_msleep(1000 * wifi_connection_retry_count); // Exponential backoff

            LOG_INF("Retrying connection (%d)\n", wifi_connection_retry_count);

            wifi_connect();
        }
    }
    else
    {
        LOG_INF("Disconnected\n");
        k_sem_take(&wifi_connected, K_NO_WAIT);
    }
}

static void handle_ipv4_result(struct net_if *iface)
{
    if (got_ipv4) {
        return;
    }

    struct net_if_config *cfg = net_if_get_config(iface);
    struct net_if_ipv4  *ipv4 = cfg->ip.ipv4;
    char buf[NET_IPV4_ADDR_LEN];
    int i;

    for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
        const struct net_if_addr_ipv4 *u = &ipv4->unicast[i];

        /* look only for the DHCP‐assigned address */
        if (u->ipv4.addr_type != NET_ADDR_DHCP) {
            continue;
        }

        /* print the IP */
        net_addr_ntop(AF_INET,
                      &u->ipv4.address.in_addr,
                      buf, sizeof(buf));

        /* print the subnet mask (now per‐entry) */
        net_addr_ntop(AF_INET,
                      &u->netmask,
                      buf, sizeof(buf));

        /* print the router/gateway */
        net_addr_ntop(AF_INET,
                      &ipv4->gw,
                      buf, sizeof(buf));

        k_sem_give(&ipv4_address_obtained);
        got_ipv4 = true;
        net_mgmt_del_event_callback(&ipv4_cb);
        break;
    }
}


static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface)
{
    switch (mgmt_event)
    {

        case NET_EVENT_WIFI_CONNECT_RESULT:
            handle_wifi_connect_result(cb);
            break;

        case NET_EVENT_WIFI_DISCONNECT_RESULT:
            handle_wifi_disconnect_result(cb);
            break;

        case NET_EVENT_IPV4_ADDR_ADD:
        case NET_EVENT_IPV4_DHCP_BOUND:
        case NET_EVENT_L4_CONNECTED:
            handle_ipv4_result(iface);
            break;

        default:
            break;
    }
}

void wifi_connect(void)
{
    struct net_if *iface = net_if_get_default();

    struct wifi_connect_req_params wifi_params = {0};

    wifi_params.ssid = CONFIG_WIFI_DEFAULT_SSID;
    wifi_params.psk = CONFIG_WIFI_DEFAULT_PSK;
    wifi_params.ssid_length = strlen(CONFIG_WIFI_DEFAULT_SSID);
    wifi_params.psk_length = strlen(CONFIG_WIFI_DEFAULT_PSK);
    wifi_params.channel = WIFI_CHANNEL_ANY;
    wifi_params.security = WIFI_SECURITY_TYPE_PSK;
    wifi_params.band = WIFI_FREQ_BAND_2_4_GHZ; 
    wifi_params.mfp = WIFI_MFP_OPTIONAL;

    LOG_DBG("Connecting to SSID: %s\n", wifi_params.ssid);

    if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &wifi_params, sizeof(struct wifi_connect_req_params)))
    {
        LOG_ERR("WiFi Connection Request Failed\n");
    }
}

void wifi_status(void)
{
    struct net_if *iface = net_if_get_default();

    struct wifi_iface_status status = {0};

    if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,	sizeof(struct wifi_iface_status)))
    {
        LOG_ERR("WiFi Status Request Failed\n");
    }

    if (status.state >= WIFI_STATE_ASSOCIATED) {
        LOG_DBG("SSID: %-32s\n", status.ssid);
        LOG_DBG("Band: %s\n", wifi_band_txt(status.band));
        LOG_DBG("Channel: %d\n", status.channel);
        LOG_DBG("Security: %s\n", wifi_security_txt(status.security));
        LOG_DBG("RSSI: %d\n", status.rssi);
    }
}

void wifi_disconnect(void)
{
    struct net_if *iface = net_if_get_default();

    if (net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0))
    {
        LOG_ERR("WiFi Disconnection Request Failed\n");
    }
}

void wifi_init(void) {
    /* Layer-2 Wi-Fi events */
    net_mgmt_init_event_callback(&wifi_cb,
        wifi_mgmt_event_handler,
        NET_EVENT_WIFI_CONNECT_RESULT   |
        NET_EVENT_WIFI_DISCONNECT_RESULT
    );
    net_mgmt_add_event_callback(&wifi_cb);

    /* Layer-3 IPv4 events only */
    net_mgmt_init_event_callback(&ipv4_cb,
        wifi_mgmt_event_handler,
        NET_EVENT_IPV4_ADDR_ADD     |
        NET_EVENT_IPV4_DHCP_BOUND
    );
    net_mgmt_add_event_callback(&ipv4_cb);

    /* Layer-4 “connectivity” event only */
    net_mgmt_init_event_callback(&l4_cb,
        wifi_mgmt_event_handler,
        NET_EVENT_L4_CONNECTED
    );

    net_mgmt_add_event_callback(&l4_cb);
}

void wifi_thread(void)
{
    LOG_INF("WiFi Thread started");

    /* Initialize the Wi-Fi interface */
    wifi_init();

    wifi_connect();
    k_sem_take(&wifi_connected, K_FOREVER);
    wifi_status();
    k_sem_take(&ipv4_address_obtained, K_FOREVER);


    LOG_INF("WiFi Thread finished");
}