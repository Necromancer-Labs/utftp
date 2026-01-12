/*
 * utftp - Session management
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../include/session.h"
#include "../include/packet.h"
#include "../include/log.h"

tftp_session_t* session_alloc(tftp_server_t *srv)
{
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (srv->sessions[i].state == STATE_FREE) {
            tftp_session_t *sess = &srv->sessions[i];
            memset(sess, 0, sizeof(*sess));
            sess->fd = -1;
            sess->sock = -1;
            sess->blksize = TFTP_DEF_BLKSIZE;
            return sess;
        }
    }
    return NULL;
}

void session_free(tftp_session_t *sess)
{
    if (sess->fd >= 0) {
        close(sess->fd);
        sess->fd = -1;
    }
    if (sess->sock >= 0) {
        close(sess->sock);
        sess->sock = -1;
    }
    sess->state = STATE_FREE;
}

tftp_session_t* session_find_by_addr(tftp_server_t *srv, struct sockaddr_in *addr)
{
    for (int i = 0; i < MAX_SESSIONS; i++) {
        tftp_session_t *sess = &srv->sessions[i];
        if (sess->state != STATE_FREE &&
            sess->client_addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
            sess->client_addr.sin_port == addr->sin_port) {
            return sess;
        }
    }
    return NULL;
}

int session_create_socket(tftp_server_t *srv)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        log_msg(LOG_ERROR, "Failed to create session socket: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = 0;

    if (srv->config.bind_addr[0]) {
        inet_pton(AF_INET, srv->config.bind_addr, &addr.sin_addr);
    } else {
        addr.sin_addr.s_addr = INADDR_ANY;
    }

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        log_msg(LOG_ERROR, "Failed to bind session socket: %s", strerror(errno));
        close(sock);
        return -1;
    }

    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    return sock;
}

int session_send_packet(tftp_session_t *sess, uint8_t *buf, size_t len)
{
    memcpy(sess->last_packet, buf, len);
    sess->last_packet_len = len;
    sess->retries = 0;
    gettimeofday(&sess->last_activity, NULL);

    ssize_t sent = sendto(sess->sock, buf, len, 0,
                          (struct sockaddr *)&sess->client_addr,
                          sizeof(sess->client_addr));

    if (sent < 0) {
        log_msg(LOG_ERROR, "sendto failed: %s", strerror(errno));
        return -1;
    }

    return 0;
}

int session_retransmit(tftp_session_t *sess)
{
    if (sess->last_packet_len == 0)
        return -1;

    sess->retries++;
    gettimeofday(&sess->last_activity, NULL);

    log_msg(LOG_DEBUG, "Retransmit #%d to %s:%d",
            sess->retries,
            inet_ntoa(sess->client_addr.sin_addr),
            ntohs(sess->client_addr.sin_port));

    ssize_t sent = sendto(sess->sock, sess->last_packet, sess->last_packet_len, 0,
                          (struct sockaddr *)&sess->client_addr,
                          sizeof(sess->client_addr));

    return (sent < 0) ? -1 : 0;
}

void session_send_error(tftp_session_t *sess, tftp_error_t code, const char *msg)
{
    uint8_t buf[512];
    int len = packet_build_error(buf, code, msg);

    sendto(sess->sock, buf, len, 0,
           (struct sockaddr *)&sess->client_addr,
           sizeof(sess->client_addr));

    log_msg(LOG_WARN, "Error to %s:%d: %s",
            inet_ntoa(sess->client_addr.sin_addr),
            ntohs(sess->client_addr.sin_port), msg);
}
