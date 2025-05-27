#include <zephyr/net/socket.h>
#include <zephyr/kernel.h>
#include <zephyr/net/http/client.h>
#include <zephyr/logging/log.h>
#include <env.h>
#include <parser.h>
#include <gui.h>

#define REQUEST_SIZE 2048

static uint8_t recv_buf[512];

/* http accumulator and its current length */
static char    full_body[REQUEST_SIZE];
static size_t  full_body_len;

LOG_MODULE_REGISTER(http_module, LOG_LEVEL_DBG);

static const char *http_headers[] = {
    "Authorization: Api-Key " API_KEY "\r\n",
    "Content-Type: application/json\r\n",
    "Content-Length: 0\r\n",
    NULL
};

int nslookup(const char * hostname, struct zsock_addrinfo **results)
{
    struct zsock_addrinfo hints = {
        .ai_family   = AF_INET,       // try IPv4 first
        .ai_socktype = SOCK_STREAM,
    };

    int ret = zsock_getaddrinfo(hostname, NULL, &hints, results);

    if (ret < 0) {
        LOG_ERR("IPv4 failed (%d), falling back to IPv6", ret);

        hints.ai_family = AF_INET6;
        ret = zsock_getaddrinfo(hostname, NULL, &hints, results);

        if (ret < 0) {
            LOG_ERR("IPv6 lookup failed (%d)", ret);
            return -1;
        }
    }

    /* now res points to a list of valid addrs in the one family that worked */
    return 0;
}

void print_addrinfo_results(struct zsock_addrinfo **results)
{
    char ipv4[INET_ADDRSTRLEN];
    char ipv6[INET6_ADDRSTRLEN];
    struct sockaddr_in *sa;
    struct sockaddr_in6 *sa6;
    struct zsock_addrinfo *rp;
    
    for (rp = *results; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_addr->sa_family == AF_INET) {
            // IPv4 Address
            sa = (struct sockaddr_in *) rp->ai_addr;
            zsock_inet_ntop(AF_INET, &sa->sin_addr, ipv4, INET_ADDRSTRLEN);
            LOG_INF("IPv4: %s\n", ipv4);
        }
        if (rp->ai_addr->sa_family == AF_INET6) {
            // IPv6 Address
            sa6 = (struct sockaddr_in6 *) rp->ai_addr;
            zsock_inet_ntop(AF_INET6, &sa6->sin6_addr, ipv6, INET6_ADDRSTRLEN);
            LOG_INF("IPv6: %s\n", ipv6);
        }
    }
}

int connect_socket(struct zsock_addrinfo **results, uint16_t port)
{
    struct zsock_addrinfo *rp;
    int sock, ret;

    for (rp = *results; rp; rp = rp->ai_next) {
        /* set the port in the sockaddr */
        if (rp->ai_family == AF_INET) {
            ((struct sockaddr_in *)rp->ai_addr)->sin_port = htons(port);
        } else if (rp->ai_family == AF_INET6) {
            ((struct sockaddr_in6 *)rp->ai_addr)->sin6_port = htons(port);
        }

        sock = zsock_socket(rp->ai_family,
                            rp->ai_socktype,
                            rp->ai_protocol);
        if (sock < 0) {
            LOG_ERR("socket() failed: %d\n", sock);
            continue;
        }

        ret = zsock_connect(sock,
                            rp->ai_addr,
                            rp->ai_addrlen);
        if (ret == 0) {
            LOG_INF("Connected on family %d\n", rp->ai_family);
            return sock;
        }

        if (ret < 0) {
            int err = errno;  /* POSIX errno */
            LOG_ERR("connect(family %d) → ret=%d, errno=%d (%s)\n",
                   rp->ai_family, ret, err, strerror(err));
            zsock_close(sock);
            continue;
        }
        zsock_close(sock);
    }

    return -1;
}


static int http_response_cb(struct http_response *rsp,
                            enum http_final_call final_data,
                            void *user_data)
{
    ARG_UNUSED(user_data);

    /* copy this chunk into full_body[] */
    if (rsp->data_len > 0) {
        if (full_body_len + rsp->data_len < sizeof(full_body) - 1) {
            memcpy(full_body + full_body_len,
                   rsp->recv_buf,
                   rsp->data_len);

            full_body_len += rsp->data_len;
        } else {
            LOG_ERR("Response too large to fit buffer");
            return -ENOMEM;
        }
    }

    if (final_data == HTTP_DATA_FINAL) {
        /* NUL-terminate and hand off to your JSON parser */
        full_body[full_body_len] = '\0';
        LOG_INF("Full JSON payload (%zu bytes):\n%s", full_body_len, full_body);

        /* now parse into your notification_t array */
        parse_notifications(full_body, full_body_len);

        /* and finally, refresh your GUI */
        #ifdef CONFIG_GUI
        gui_show_notifications_screen();
        #endif
    }

    return 0;
}

int http_get(int sock, char * hostname, char * url)
{
    struct http_request req = { 0 };
    int ret;

    req.method = HTTP_GET;
    req.url = url;
    req.host = hostname;
    req.port = "3000";
    req.protocol = "HTTP/1.1";
    req.response = http_response_cb;
    req.recv_buf = recv_buf;
    req.recv_buf_len = sizeof(recv_buf);
    req.header_fields = http_headers;

    LOG_DBG("HTTP Request: %s %s%s %s\n", req.method == HTTP_GET ? "GET" : "POST", req.host, req.url, req.protocol);

    /* sock is a file descriptor referencing a socket that has been connected
    * to the HTTP server.
    */
    ret = http_client_req(sock, &req, 5000, NULL);

    return ret;
}

int http_query(char * hostname, char * url) {
    int sock;

    struct zsock_addrinfo *res;
    nslookup(hostname, &res);
    print_addrinfo_results(&res);

    sock = connect_socket(&res, 3000);
    int ret = http_get(sock, hostname, url);

    if (ret < 0) {
        LOG_ERR("HTTP GET failed: %d", ret);
    }

    zsock_freeaddrinfo(res);
    zsock_close(sock);

    return ret;
}
