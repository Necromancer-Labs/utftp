/*
 * utftp - Transfer handlers
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include "../include/transfer.h"
#include "../include/session.h"
#include "../include/packet.h"
#include "../include/util.h"
#include "../include/log.h"

int handle_rrq(tftp_server_t *srv, tftp_session_t *sess, uint8_t *buf, size_t len)
{
    char filename[MAX_FILENAME_LEN];
    char mode[32];
    size_t blksize, tsize;

    if (packet_parse_request(buf, len, filename, sizeof(filename),
                             mode, sizeof(mode), &blksize, &tsize) < 0) {
        session_send_error(sess, TFTP_ERR_ILLEGAL_OP, "Malformed request");
        return -1;
    }

    if (strcasecmp(mode, "octet") != 0 && strcasecmp(mode, "netascii") != 0) {
        session_send_error(sess, TFTP_ERR_ILLEGAL_OP, "Unsupported mode");
        return -1;
    }

    char fullpath[MAX_PATH_LEN];
    if (validate_path(srv->config.root_dir, filename, fullpath, sizeof(fullpath)) < 0) {
        session_send_error(sess, TFTP_ERR_ACCESS_DENIED, "Access denied");
        return -1;
    }

    sess->fd = open(fullpath, O_RDONLY);
    if (sess->fd < 0) {
        session_send_error(sess, TFTP_ERR_FILE_NOT_FOUND, "File not found");
        return -1;
    }

    struct stat st;
    if (fstat(sess->fd, &st) == 0) {
        sess->tsize = st.st_size;
    }

    strncpy(sess->filename, filename, sizeof(sess->filename) - 1);
    sess->blksize = blksize;
    sess->block_num = 0;
    sess->state = STATE_SENDING;
    gettimeofday(&sess->start_time, NULL);

    char sizebuf[32];
    if (g_use_color) {
        log_msg(LOG_INFO, "%s<-- GET%s %s%s%s (%s) from %s%s:%d%s",
                C_GREEN, C_RESET,
                C_BOLD, filename, C_RESET,
                format_size(sess->tsize, sizebuf, sizeof(sizebuf)),
                C_MAGENTA, inet_ntoa(sess->client_addr.sin_addr),
                ntohs(sess->client_addr.sin_port), C_RESET);
    } else {
        log_msg(LOG_INFO, "<-- GET %s (%s) from %s:%d",
                filename,
                format_size(sess->tsize, sizebuf, sizeof(sizebuf)),
                inet_ntoa(sess->client_addr.sin_addr),
                ntohs(sess->client_addr.sin_port));
    }

    uint8_t pkt[TFTP_MAX_PACKET];
    int pkt_len;

    if (blksize != TFTP_DEF_BLKSIZE || tsize != 0) {
        pkt_len = packet_build_oack(pkt, blksize, sess->tsize, 1);
        return session_send_packet(sess, pkt, pkt_len);
    } else {
        sess->block_num = 1;
        uint8_t data[TFTP_MAX_BLKSIZE];
        ssize_t n = read(sess->fd, data, sess->blksize);
        if (n < 0) {
            session_send_error(sess, TFTP_ERR_UNDEFINED, "Read error");
            return -1;
        }

        pkt_len = packet_build_data(pkt, sess->block_num, data, n);
        sess->bytes_transferred = n;

        if ((size_t)n < sess->blksize) {
            sess->state = STATE_LAST_DATA;
        }

        return session_send_packet(sess, pkt, pkt_len);
    }
}

int handle_wrq(tftp_server_t *srv, tftp_session_t *sess, uint8_t *buf, size_t len)
{
    char filename[MAX_FILENAME_LEN];
    char mode[32];
    size_t blksize, tsize;

    if (packet_parse_request(buf, len, filename, sizeof(filename),
                             mode, sizeof(mode), &blksize, &tsize) < 0) {
        session_send_error(sess, TFTP_ERR_ILLEGAL_OP, "Malformed request");
        return -1;
    }

    if (strcasecmp(mode, "octet") != 0 && strcasecmp(mode, "netascii") != 0) {
        session_send_error(sess, TFTP_ERR_ILLEGAL_OP, "Unsupported mode");
        return -1;
    }

    char fullpath[MAX_PATH_LEN];
    if (validate_path(srv->config.root_dir, filename, fullpath, sizeof(fullpath)) < 0) {
        session_send_error(sess, TFTP_ERR_ACCESS_DENIED, "Access denied");
        return -1;
    }

    /* Create parent directories if needed */
    char *slash = strrchr(fullpath, '/');
    if (slash) {
        *slash = '\0';
        char tmp[MAX_PATH_LEN];
        strncpy(tmp, fullpath, sizeof(tmp) - 1);
        for (char *p = tmp + 1; *p; p++) {
            if (*p == '/') {
                *p = '\0';
                mkdir(tmp, 0755);
                *p = '/';
            }
        }
        mkdir(tmp, 0755);
        *slash = '/';
    }

    sess->fd = open(fullpath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (sess->fd < 0) {
        session_send_error(sess, TFTP_ERR_ACCESS_DENIED, "Cannot create file");
        return -1;
    }

    strncpy(sess->filename, filename, sizeof(sess->filename) - 1);
    sess->blksize = blksize;
    sess->tsize = tsize;
    sess->block_num = 0;
    sess->state = STATE_RECEIVING;
    gettimeofday(&sess->start_time, NULL);

    if (g_use_color) {
        log_msg(LOG_INFO, "%s--> PUT%s %s%s%s from %s%s:%d%s",
                C_YELLOW, C_RESET,
                C_BOLD, filename, C_RESET,
                C_MAGENTA, inet_ntoa(sess->client_addr.sin_addr),
                ntohs(sess->client_addr.sin_port), C_RESET);
    } else {
        log_msg(LOG_INFO, "--> PUT %s from %s:%d",
                filename,
                inet_ntoa(sess->client_addr.sin_addr),
                ntohs(sess->client_addr.sin_port));
    }

    uint8_t pkt[512];
    int pkt_len;

    if (blksize != TFTP_DEF_BLKSIZE || tsize != 0) {
        pkt_len = packet_build_oack(pkt, blksize, tsize, tsize != 0);
    } else {
        pkt_len = packet_build_ack(pkt, 0);
    }

    return session_send_packet(sess, pkt, pkt_len);
}

int handle_ack(tftp_session_t *sess, uint8_t *buf, size_t len)
{
    if (len < 4)
        return -1;

    uint16_t ack_block = (buf[2] << 8) | buf[3];

    log_msg(LOG_DEBUG, "ACK %d from %s:%d",
            ack_block,
            inet_ntoa(sess->client_addr.sin_addr),
            ntohs(sess->client_addr.sin_port));

    if (ack_block == 0 && sess->block_num == 0) {
        sess->block_num = 1;
    }
    else if (ack_block == sess->block_num) {
        if (sess->state == STATE_LAST_DATA) {
            struct timeval now;
            gettimeofday(&now, NULL);
            double elapsed = (now.tv_sec - sess->start_time.tv_sec) +
                           (now.tv_usec - sess->start_time.tv_usec) / 1000000.0;
            if (elapsed < 0.001) elapsed = 0.001;
            double speed = sess->bytes_transferred / elapsed;

            char sizebuf[32], speedbuf[32];
            if (g_use_color) {
                log_msg(LOG_INFO, "%sSENT SUCCESS%s %s%s%s %s @ %s to %s%s:%d%s",
                        C_GREEN C_BOLD, C_RESET,
                        C_BOLD, sess->filename, C_RESET,
                        format_size(sess->bytes_transferred, sizebuf, sizeof(sizebuf)),
                        format_speed(speed, speedbuf, sizeof(speedbuf)),
                        C_MAGENTA, inet_ntoa(sess->client_addr.sin_addr),
                        ntohs(sess->client_addr.sin_port), C_RESET);
            } else {
                log_msg(LOG_INFO, "SENT SUCCESS %s %s @ %s to %s:%d",
                        sess->filename,
                        format_size(sess->bytes_transferred, sizebuf, sizeof(sizebuf)),
                        format_speed(speed, speedbuf, sizeof(speedbuf)),
                        inet_ntoa(sess->client_addr.sin_addr),
                        ntohs(sess->client_addr.sin_port));
            }
            return 1;
        }
        sess->block_num++;
    }
    else if (ack_block < sess->block_num) {
        gettimeofday(&sess->last_activity, NULL);
        return 0;
    }
    else {
        session_send_error(sess, TFTP_ERR_ILLEGAL_OP, "Invalid ACK");
        return -1;
    }

    uint8_t data[TFTP_MAX_BLKSIZE];
    ssize_t n = read(sess->fd, data, sess->blksize);
    if (n < 0) {
        session_send_error(sess, TFTP_ERR_UNDEFINED, "Read error");
        return -1;
    }

    uint8_t pkt[TFTP_MAX_PACKET];
    int pkt_len = packet_build_data(pkt, sess->block_num, data, n);
    sess->bytes_transferred += n;

    if ((size_t)n < sess->blksize) {
        sess->state = STATE_LAST_DATA;
    }

    return session_send_packet(sess, pkt, pkt_len);
}

int handle_data(tftp_session_t *sess, uint8_t *buf, size_t len)
{
    if (len < 4)
        return -1;

    uint16_t block = (buf[2] << 8) | buf[3];
    size_t data_len = len - 4;

    log_msg(LOG_DEBUG, "DATA %d (%zu bytes) from %s:%d",
            block, data_len,
            inet_ntoa(sess->client_addr.sin_addr),
            ntohs(sess->client_addr.sin_port));

    if (block == sess->block_num + 1) {
        if (data_len > 0) {
            ssize_t written = write(sess->fd, buf + 4, data_len);
            if (written < 0 || (size_t)written != data_len) {
                session_send_error(sess, TFTP_ERR_DISK_FULL, "Write error");
                return -1;
            }
        }

        sess->block_num = block;
        sess->bytes_transferred += data_len;

        uint8_t pkt[4];
        int pkt_len = packet_build_ack(pkt, block);
        session_send_packet(sess, pkt, pkt_len);

        if (data_len < sess->blksize) {
            struct timeval now;
            gettimeofday(&now, NULL);
            double elapsed = (now.tv_sec - sess->start_time.tv_sec) +
                           (now.tv_usec - sess->start_time.tv_usec) / 1000000.0;
            if (elapsed < 0.001) elapsed = 0.001;
            double speed = sess->bytes_transferred / elapsed;

            char sizebuf[32], speedbuf[32];
            if (g_use_color) {
                log_msg(LOG_INFO, "%sRECV SUCCESS%s %s%s%s %s @ %s from %s%s:%d%s",
                        C_YELLOW C_BOLD, C_RESET,
                        C_BOLD, sess->filename, C_RESET,
                        format_size(sess->bytes_transferred, sizebuf, sizeof(sizebuf)),
                        format_speed(speed, speedbuf, sizeof(speedbuf)),
                        C_MAGENTA, inet_ntoa(sess->client_addr.sin_addr),
                        ntohs(sess->client_addr.sin_port), C_RESET);
            } else {
                log_msg(LOG_INFO, "RECV SUCCESS %s %s @ %s from %s:%d",
                        sess->filename,
                        format_size(sess->bytes_transferred, sizebuf, sizeof(sizebuf)),
                        format_speed(speed, speedbuf, sizeof(speedbuf)),
                        inet_ntoa(sess->client_addr.sin_addr),
                        ntohs(sess->client_addr.sin_port));
            }
            return 1;
        }
    }
    else if (block <= sess->block_num) {
        uint8_t pkt[4];
        int pkt_len = packet_build_ack(pkt, block);
        sendto(sess->sock, pkt, pkt_len, 0,
               (struct sockaddr *)&sess->client_addr,
               sizeof(sess->client_addr));
    }
    else {
        session_send_error(sess, TFTP_ERR_ILLEGAL_OP, "Invalid block number");
        return -1;
    }

    return 0;
}

int process_session_packet(tftp_session_t *sess, uint8_t *buf, size_t len)
{
    if (len < 2)
        return -1;

    uint16_t opcode = (buf[0] << 8) | buf[1];

    switch (sess->state) {
        case STATE_SENDING:
        case STATE_LAST_DATA:
            if (opcode == TFTP_ACK) {
                return handle_ack(sess, buf, len);
            } else if (opcode == TFTP_ERROR) {
                log_msg(LOG_WARN, "Client error: %s", buf + 4);
                return -1;
            }
            break;

        case STATE_RECEIVING:
            if (opcode == TFTP_DATA) {
                return handle_data(sess, buf, len);
            } else if (opcode == TFTP_ERROR) {
                log_msg(LOG_WARN, "Client error: %s", buf + 4);
                return -1;
            }
            break;

        default:
            break;
    }

    session_send_error(sess, TFTP_ERR_ILLEGAL_OP, "Unexpected packet");
    return -1;
}
