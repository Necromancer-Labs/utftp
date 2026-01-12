/*
 * utftp - Server core
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include "../include/server.h"
#include "../include/session.h"
#include "../include/transfer.h"
#include "../include/packet.h"
#include "../include/log.h"

static int handle_new_request(tftp_server_t *srv, uint8_t *buf, size_t len,
                              struct sockaddr_in *client_addr)
{
    if (len < 2)
        return -1;

    uint16_t opcode = (buf[0] << 8) | buf[1];

    tftp_session_t *sess = session_alloc(srv);
    if (!sess) {
        log_msg(LOG_ERROR, "No free sessions available");
        uint8_t errbuf[64];
        int errlen = packet_build_error(errbuf, TFTP_ERR_UNDEFINED, "Server busy");
        sendto(srv->main_sock, errbuf, errlen, 0,
               (struct sockaddr *)client_addr, sizeof(*client_addr));
        return -1;
    }

    sess->sock = session_create_socket(srv);
    if (sess->sock < 0) {
        session_free(sess);
        return -1;
    }

    memcpy(&sess->client_addr, client_addr, sizeof(sess->client_addr));
    gettimeofday(&sess->last_activity, NULL);

    int result;
    if (opcode == TFTP_RRQ) {
        result = handle_rrq(srv, sess, buf, len);
    } else if (opcode == TFTP_WRQ) {
        result = handle_wrq(srv, sess, buf, len);
    } else {
        session_send_error(sess, TFTP_ERR_ILLEGAL_OP, "Expected RRQ or WRQ");
        session_free(sess);
        return -1;
    }

    if (result < 0) {
        session_free(sess);
        return -1;
    }

    return 0;
}

int tftp_server_init(tftp_server_t *srv, tftp_config_t *config)
{
    memset(srv, 0, sizeof(*srv));
    memcpy(&srv->config, config, sizeof(srv->config));

    for (int i = 0; i < MAX_SESSIONS; i++) {
        srv->sessions[i].state = STATE_FREE;
        srv->sessions[i].fd = -1;
        srv->sessions[i].sock = -1;
    }

    srv->main_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (srv->main_sock < 0) {
        log_msg(LOG_CRITICAL, "Failed to create socket: %s", strerror(errno));
        return -1;
    }

    int opt = 1;
    setsockopt(srv->main_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config->port);

    if (config->bind_addr[0]) {
        inet_pton(AF_INET, config->bind_addr, &addr.sin_addr);
    } else {
        addr.sin_addr.s_addr = INADDR_ANY;
    }

    if (bind(srv->main_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log_msg(LOG_CRITICAL, "Failed to bind to %s:%d: %s",
                config->bind_addr[0] ? config->bind_addr : "0.0.0.0",
                config->port, strerror(errno));
        close(srv->main_sock);
        return -1;
    }

    int flags = fcntl(srv->main_sock, F_GETFL, 0);
    fcntl(srv->main_sock, F_SETFL, flags | O_NONBLOCK);

    srv->running = 1;

    print_banner();

    if (g_use_color) {
        log_msg(LOG_INFO, "Listening on %s%s:%d%s",
                C_GREEN C_BOLD,
                config->bind_addr[0] ? config->bind_addr : "0.0.0.0",
                config->port, C_RESET);
        log_msg(LOG_INFO, "Serving from %s%s%s",
                C_BOLD, config->root_dir, C_RESET);
        log_msg(LOG_INFO, "Ready for connections (max %d concurrent)", MAX_SESSIONS);
    } else {
        log_msg(LOG_INFO, "Listening on %s:%d",
                config->bind_addr[0] ? config->bind_addr : "0.0.0.0",
                config->port);
        log_msg(LOG_INFO, "Serving from %s", config->root_dir);
        log_msg(LOG_INFO, "Ready for connections (max %d concurrent)", MAX_SESSIONS);
    }

    return 0;
}

int tftp_server_run(tftp_server_t *srv)
{
    uint8_t buf[TFTP_MAX_PACKET];
    struct timeval timeout;

    while (srv->running) {
        fd_set readfds;
        FD_ZERO(&readfds);

        int maxfd = srv->main_sock;
        FD_SET(srv->main_sock, &readfds);

        for (int i = 0; i < MAX_SESSIONS; i++) {
            if (srv->sessions[i].state != STATE_FREE && srv->sessions[i].sock >= 0) {
                FD_SET(srv->sessions[i].sock, &readfds);
                if (srv->sessions[i].sock > maxfd)
                    maxfd = srv->sessions[i].sock;
            }
        }

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int ready = select(maxfd + 1, &readfds, NULL, NULL, &timeout);

        if (ready < 0) {
            if (errno == EINTR)
                continue;
            log_msg(LOG_ERROR, "select failed: %s", strerror(errno));
            break;
        }

        if (FD_ISSET(srv->main_sock, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t addrlen = sizeof(client_addr);

            ssize_t n = recvfrom(srv->main_sock, buf, sizeof(buf), 0,
                                 (struct sockaddr *)&client_addr, &addrlen);

            if (n > 0) {
                handle_new_request(srv, buf, n, &client_addr);
            }
        }

        struct timeval now;
        gettimeofday(&now, NULL);

        for (int i = 0; i < MAX_SESSIONS; i++) {
            tftp_session_t *sess = &srv->sessions[i];
            if (sess->state == STATE_FREE)
                continue;

            if (sess->sock >= 0 && FD_ISSET(sess->sock, &readfds)) {
                struct sockaddr_in from_addr;
                socklen_t addrlen = sizeof(from_addr);

                ssize_t n = recvfrom(sess->sock, buf, sizeof(buf), 0,
                                     (struct sockaddr *)&from_addr, &addrlen);

                if (n > 0) {
                    if (from_addr.sin_addr.s_addr != sess->client_addr.sin_addr.s_addr ||
                        from_addr.sin_port != sess->client_addr.sin_port) {
                        uint8_t errbuf[64];
                        int errlen = packet_build_error(errbuf, TFTP_ERR_UNKNOWN_TID, "Unknown TID");
                        sendto(sess->sock, errbuf, errlen, 0,
                               (struct sockaddr *)&from_addr, sizeof(from_addr));
                        continue;
                    }

                    int result = process_session_packet(sess, buf, n);
                    if (result != 0) {
                        session_free(sess);
                    }
                }
            }

            if (sess->state != STATE_FREE) {
                long elapsed = (now.tv_sec - sess->last_activity.tv_sec);
                if (elapsed >= srv->config.timeout_sec) {
                    if (sess->retries >= TFTP_MAX_RETRIES) {
                        log_msg(LOG_WARN, "Session timeout: %s from %s:%d",
                                sess->filename,
                                inet_ntoa(sess->client_addr.sin_addr),
                                ntohs(sess->client_addr.sin_port));
                        session_free(sess);
                    } else {
                        session_retransmit(sess);
                    }
                }
            }
        }
    }

    return 0;
}

void tftp_server_stop(tftp_server_t *srv)
{
    srv->running = 0;
}

void tftp_server_cleanup(tftp_server_t *srv)
{
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (srv->sessions[i].state != STATE_FREE) {
            session_free(&srv->sessions[i]);
        }
    }

    if (srv->main_sock >= 0) {
        close(srv->main_sock);
        srv->main_sock = -1;
    }
}
