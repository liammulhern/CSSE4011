#include <zephyr/net/socket.h>
#include <zephyr/kernel.h>
#include <zephyr/net/http/client.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/http/parser.h>
#include <env.h>
#include <parser.h>
#include <gui.h>

#define REQUEST_SIZE 2048
#define BODY_BUF_SIZE 2048

static uint8_t recv_buf[1024];

LOG_MODULE_REGISTER(http_module, LOG_LEVEL_DBG);

static const char *http_headers[] = {
    "Authorization: Api-Key " API_KEY "\r\n",
    "Content-Type: application/json\r\n",
    "Content-Length: 0\r\n",
    NULL
};

/*  Parser state + settings */
static struct http_parser parser;
static struct http_parser_settings parser_settings;

/* Body accumulator */
static char body_buf[BODY_BUF_SIZE];
static size_t body_buf_len;

/* Called at the start of each message */
static int on_message_begin(struct http_parser *p) {
    ARG_UNUSED(p);
    body_buf_len = 0;
    return 0;
}

/* We don’t need URL, status or headers here, so leave them NULL */
/* Accumulate each body chunk into body_buf[] */
static int on_body(struct http_parser *p, const char *at, size_t length) {
    ARG_UNUSED(p);

    if (body_buf_len + length < BODY_BUF_SIZE - 1) {
        memcpy(body_buf + body_buf_len, at, length);
        body_buf_len += length;
    } else {
        LOG_ERR("Body too large (%zu + %zu > %zu)",
                body_buf_len, length, sizeof(body_buf));
        return -1;
    }

    return 0;
}

/* When the full message is done, NUL‐terminate and hand off */
static int on_message_complete(struct http_parser *p) {
    ARG_UNUSED(p);
    body_buf[body_buf_len] = '\0';
    LOG_INF("Full JSON payload (%zu bytes)", body_buf_len);

    /* parse and redraw GUI */
    parse_notifications(body_buf, body_buf_len);
    gui_notify_notifications_update();

    return 0;
}

static int parser_init_once(void)
{
    http_parser_init(&parser, HTTP_RESPONSE);
    http_parser_settings_init(&parser_settings);

    parser_settings.on_message_begin = on_message_begin;
    parser_settings.on_body = on_body;
    parser_settings.on_message_complete = on_message_complete;

    return 0;
}

/* Call this at startup, once */
SYS_INIT(parser_init_once, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

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

     if (rsp->data_len > 0) {
        http_parser_execute(&parser,
                            &parser_settings,
                            (const char *)rsp->recv_buf,
                            rsp->data_len);
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
