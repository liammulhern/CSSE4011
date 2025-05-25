/* 
 * Copyright (c) 2023 Craig Peacock.
 * 
 * API Information:
 * https://docs.zephyrproject.org/3.2.0/connectivity/networking/api/http.html
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/net/socket.h>
#include <zephyr/kernel.h>
#include <zephyr/net/http/client.h>

int nslookup(const char * hostname, struct zsock_addrinfo **results)
{
    struct zsock_addrinfo hints = {
        .ai_family   = AF_INET,       // try IPv4 first
        .ai_socktype = SOCK_STREAM,
    };

    int ret = zsock_getaddrinfo(hostname, NULL, &hints, results);

    if (ret < 0) {
        printk("IPv4 lookup failed (%d), falling back to IPv6\n", ret);

        hints.ai_family = AF_INET6;
        ret = zsock_getaddrinfo(hostname, NULL, &hints, results);

        if (ret < 0) {
            printk("IPv6 lookup also failed (%d)\n", ret);
            return -1;
        }
    }

    /* now res points to a list of valid addrs in the one family that worked */
    return 0;
}

void print_addrinfo_results(struct zsock_addrinfo **results)
{
    printk("Address Information:\n");
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
            printk("IPv4: %s\n", ipv4);
        }
        if (rp->ai_addr->sa_family == AF_INET6) {
            // IPv6 Address
            sa6 = (struct sockaddr_in6 *) rp->ai_addr;
            zsock_inet_ntop(AF_INET6, &sa6->sin6_addr, ipv6, INET6_ADDRSTRLEN);
            printk("IPv6: %s\n", ipv6);
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
            printk("socket() failed: %d\n", sock);
            continue;
        }

        ret = zsock_connect(sock,
                            rp->ai_addr,
                            rp->ai_addrlen);
        if (ret == 0) {
            printk("Connected on family %d\n", rp->ai_family);
            return sock;
        }

        if (ret < 0) {
            int err = errno;  /* POSIX errno */
            printk("connect(family %d) → ret=%d, errno=%d (%s)\n",
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
    if (final_data == HTTP_DATA_MORE) {
        //printk("Partial data received (%zd bytes)\n", rsp->data_len);
    } else if (final_data == HTTP_DATA_FINAL) {
        //printk("All the data received (%zd bytes)\n", rsp->data_len);
    }

    //printk("Bytes Recv %zd\n", rsp->data_len);
    //printk("Response status %s\n", rsp->http_status);
    //printk("Recv Buffer Length %zd\n", rsp->recv_buf_len);
    printk("%.*s", rsp->data_len, rsp->recv_buf);
}

void http_get(int sock, char * hostname, char * url)
{
    struct http_request req = { 0 };
    static uint8_t recv_buf[512];
    int ret;

    req.method = HTTP_GET;
    req.url = url;
    req.host = hostname;
    req.protocol = "HTTP/1.1";
    req.response = http_response_cb;
    req.recv_buf = recv_buf;
    req.recv_buf_len = sizeof(recv_buf);

    /* sock is a file descriptor referencing a socket that has been connected
    * to the HTTP server.
    */
    ret = http_client_req(sock, &req, 5000, NULL);
}
